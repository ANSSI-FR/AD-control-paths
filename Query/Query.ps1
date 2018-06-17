#
# Script parameters & variables
# 
Param(
    [string]$outFile = $null,
    [string]$search = "cn=domain admins,",
    [int]$graph = $null,
    [switch]$quick = $false,
    [int]$maxDepth = 2,
    
    [switch]$help = $false,
    
    [string]$neo4jHost = "localhost",
    [int]$neo4jPort = 7474,
    [string]$neo4jUser = "neo4j",
    [string]$neo4jPassword = "secret"
    
    )
    
$date =  date -Format yyyyMMdd

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
    Write-Output "`t-quick`t: search and graph cn=domain admins"
    Write-Output "`t-search <NODENAME>`t: search from node distinguished name"
    Write-Output "`t-graph <NODEID>`t: generates a JSON control path graph from a node id"
      
    Write-Output "- Optional parameters:"
    Write-Output "`t-outFile <FILENAME>         : (default: console output)"    
    Write-Output "`t-maxDepth <MAXDEPTH>         : (default: 2)"
    
    Write-Output "`t-neo4jHost <NEO4JSRV>       : (default: localhost)"
    Write-Output "`t-neo4jPort <NEO4JPORT>         : (default: 7474/tcp)"
    Write-Output "`t-neo4jUser <NEO4JUSR>     : (default: neo4j)"
    Write-Output "`t-neo4jPassword <NEO4PWD>     : (default: secret)"
    Write-Output "`t-neo4jPromptCreds      : Prompts for neo4j credentials (default: no)"
    


    Break
}

# Parameters checking
If([int]$PSBoundParameters.ContainsKey('search') + [int]$PSBoundParameters.ContainsKey('graph') + [int]$PSBoundParameters.ContainsKey('quick') -ne 1) {
    Usage "We need exactly one of -quick, -search, -graph."
}

# Authentication
If($neo4jPromptCreds.IsPresent) {
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

# Cypher search
If( $PSBoundParameters.ContainsKey('search') -or $PSBoundParameters.ContainsKey('quick') ) {
$Body='{"statements":[{"statement" : "match(n) WHERE n.name STARTS WITH $searchedName WITH {id:ID(n),name:n.name} AS principal RETURN principal","parameters" : {"searchedName" : "'+$search+'"} }]}'
try{
$searchResult = Invoke-RestMethod -Method Post  -Uri $Uri -Credential $Cred -Headers $Headers  -Body $Body #-OutFile $outFile
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

$searchResult.results.data.row #| ConvertTo-Json

}

# Cypher graph
If( $PSBoundParameters.ContainsKey('quick') ) {
If($searchResult.results.data.row.Count -eq 0) {
Usage "Could not find cn=domain admins, -search for something else"
}
Write-Output "`n[*]Found domain admins group"
$graph = $searchResult.results.data.row[0].id

}

If( $PSBoundParameters.ContainsKey('graph') -or $PSBoundParameters.ContainsKey('quick') ) {
Write-Output "[*]Querying for control graph of node $graph"
# We cannot parameterize shortestPath depth as it is part of the query planning
$Body = '{"statements":[{"statement" : "match path = shortestPath( (n)<-[*..'+$maxDepth+']-(m) ) where id(n)=$graphedNode return path","parameters" : {"graphedNode" : '+$graph+'} , "resultDataContents" : [ "row", "graph" ] }]}'
#$Body = '{"statements":[{"statement" : "match path = shortestPath( (n)<-[*..$depth]-(m) ) where id(n)=$graphedNode return path","parameters" : {"graphedNode" : '+$graph+',"depth" : '+$maxDepth+'} , "resultDataContents" : [ "row", "graph" ] }]}'
#$Body
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


If ( $PSBoundParameters.ContainsKey('outFile') ) {
Write-Output "With outfile"
$searchResult.results.data.graph | ConvertTo-Json -depth 100 | Out-File $outFile
}
Else {
Write-Output "Without outfile, displaying results"
$searchResult.results.data.graph | ConvertTo-Json  -depth 100
}
}

