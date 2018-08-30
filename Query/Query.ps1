#
# Script parameters & variables
# 
Param(    
    [switch]$help = $false,
    [string]$outFile = ".\out.json",
	[switch]$console = $false,
    [string]$search = "cn=admins du domaine,",
    [int]$graph = $null,
    [switch]$quick = $false,
    [int]$maxDepth = 15,    
    [string]$neo4jHost = "localhost",
    [int]$neo4jPort = 7474,
    [string]$neo4jUser = "neo4j",
    [string]$neo4jPassword = "secret"   
    )
    
#
# Verifying script options
# 
Function Usage([string]$errmsg = $null)
{
    if($errmsg) {
        Write-Output "Error: $errmsg"
    }
    Write-Output "Usage: $(Split-Path -Leaf $PSCommandPath) [PARAMETERS]"
    Write-Output "`t-help`t: show this help"
    
    Write-Output "- One of the following (Usually search then graph from results):"
    Write-Output "`t-quick`t: search and graph cn=domain admins to out.json"
    Write-Output "`t-search <NODENAME>`t: search from node distinguished name"
    Write-Output "`t-graph <NODEID> -outFile <FILENAME>`t: generates a JSON control path graph from a node id"
      
    Write-Output "- Optional parameters:"
    Write-Output "`t-console         : output json to console"    
    Write-Output "`t-maxDepth <MAXDEPTH>         : (default: 15)"
    Write-Output "`t-neo4jHost <NEO4JSRV>       : (default: localhost)"
    Write-Output "`t-neo4jPort <NEO4JPORT>         : (default: 7474/tcp)"
    Write-Output "`t-neo4jUser <NEO4JUSR>     : (default: neo4j)"
    Write-Output "`t-neo4jPassword <NEO4PWD>     : (default: secret)"
    Write-Output "`t-neo4jPromptCreds      : Prompts for neo4j credentials (default: no)"
    


    Break
}

Function Execute-Invoke-RestMethod {
try{
  $searchResult = Invoke-RestMethod -Method Post  -Uri $Uri -Credential $Cred -Headers $Headers  -Body $Body
}
catch{
  "[!]Invoke-RestMethod error:"
  $Error[0].Exception
  "Exiting"
  Exit
}
If($searchResult.errors) {
  "[!]Neo4j returned an error:"
  $searchResult.errors.message
  "Exiting"
  Exit
}
If(!$searchResult.results.data) {
  Write-Output "[!]Empty result"
  Exit
}
Return $searchResult.results.data.row 
}

# Parameters checking
If([int]$PSBoundParameters.ContainsKey('search') + [int]$PSBoundParameters.ContainsKey('graph') + [int]$PSBoundParameters.ContainsKey('quick') -ne 1) {
    Usage "We need exactly one of -quick, -search, -graph."
}
If($PSBoundParameters.ContainsKey('graph') -Xor $PSBoundParameters.ContainsKey('outFile')) {
    Usage "Define an output file for graph json with -outFile."
}

# Authentication
If($PSBoundParameters.ContainsKey('neo4jPromptCreds')) {
  $Cred = Get-Credential
}
Else {
  $Pwd = ConvertTo-SecureString $neo4jPassword -AsPlainText -Force
  $Cred = New-Object Management.Automation.PSCredential ($neo4jUser, $Pwd)
}
# allow the use of self-signed SSL certificates.
[System.Net.ServicePointManager]::ServerCertificateValidationCallback = { $True }

$Uri = "http://"+$neo4jHost+":"+$neo4jPort+"/db/data/transaction/commit"
# X-Stream allows neo4j to stream JSON result which improves performance and reduces memory consumption significantly server-side
# We still have a 2GB limit on the total result because of .NET memory buffer
# This can be modified in app.config, however the resulting JSON should be no more than a few MB usually.
$Headers = @{"Accept"="application/json; charset=UTF-8";"Content-Type"="application/json";"X-Stream"="true"}

# Cypher search mode
If( $PSBoundParameters.ContainsKey('search') -or $PSBoundParameters.ContainsKey('quick') ) {
  $Body = @" 
{"statements":[{"statement" : "
MATCH(n) 
WHERE n.name STARTS WITH `$searchedName 
WITH {id:ID(n),name:n.name} AS principal 
RETURN principal
","parameters" : {"searchedName" : "$search"} }]}
"@ -replace "`r`n"
  $found = Execute-Invoke-RestMethod -Method Post  -Uri $Uri -Credential $Cred -Headers $Headers  -Body $Body
  $found
}

# Quick mode matching
If( $PSBoundParameters.ContainsKey('quick') ) {
  If($found.Count -eq 0) {
    Usage "Could not find cn=domain admins, -search for something else"
  }
  Write-Output "`n[*]Found domain admins group"
  $graph = $found[0].id
}

If( $PSBoundParameters.ContainsKey('graph') -or $PSBoundParameters.ContainsKey('quick') ) {
# Only include RBAC relations if node type is email
  Write-Output "[*]Querying for type of node $graph"
  $Body = @" 
{"statements":[{"statement" : " 
MATCH (n) 
WHERE id(n)=`$graphedNode 
RETURN labels(n)[0] 
","parameters" : {"graphedNode" : $graph} , "resultDataContents" : ["row"] }]} 
"@ -replace "`r`n"

  $type = Execute-Invoke-RestMethod -Method Post  -Uri $Uri -Credential $Cred -Headers $Headers  -Body $Body
  Write-Output "[*]Node $graph type is: $type"
  $queryMail = [bool]($type -like "email" )
If($queryMail) {
  Write-Output "[*]Querying for control graph of type email"
  $RBACWhere =  ""
}
Else {
  Write-Output "[*]Querying for control graph of generic AD node"
  $RBACWhere = " AND NONE (r IN relationships(p) WHERE type(r) IN ['RBAC_SET_MBX','RBAC_CONNECT_MBX','RBAC_ADD_MBXPERM','RBAC_NEW_MBXEXPORTREQ','RBAC_SET_MBXFOLDERPERM','RBAC_ADD_MBXFOLDERPERM','RBAC_NEW_AUTHSERVER']) "
}

# Cypher graph mode query
# We cannot parameterize shortestPath depth as it is part of the query planning
$Body = @"
{"statements":[{"statement" : "
MATCH p = allShortestPaths( (n)<-[*..$maxDepth]-(u) ) 
WHERE id(n)=`$graphedNode 
$RBACWhere 
UNWIND relationships(p) as links_list 
WITH 
{ 
  source: id(startNode(links_list)),
  target: id(endNode(links_list)),
  rels:    COLLECT( distinct type(links_list) )
} AS all_links,
{
  id: id(n),
  name: n.name,
  type: labels(n)[0],
  shortname: split(n.name,\",\")[0],
  dist: 0
} AS orig_node,
{
  id: id(u),
  name: u.name,
  type: labels(u)[0],
  shortname: split(u.name,\",\")[0],
  dist: length(p)
} AS all_nodes 
RETURN {
nodes: orig_node + COLLECT(distinct all_nodes),
links: COLLECT(all_links)
}",
"parameters" : {"graphedNode" : $graph} , "resultDataContents" : ["row"] }]}
"@ -replace "`r`n"
$graphResult = Execute-Invoke-RestMethod -Method Post  -Uri $Uri -Credential $Cred -Headers $Headers  -Body $Body
If ( $PSBoundParameters.ContainsKey('console') ) {
  $graphResult[0] | ConvertTo-Json -depth 100 
}
Write-Output "[*]Writing result to Json file $outFile"
$graphResult[0] | ConvertTo-Json  -depth 100 | Out-File $outFile
}

