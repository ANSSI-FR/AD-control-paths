<#
.SYNOPSIS

Author: Geraud de Drouas
Inspired (and some code) by https://ingogegenwarth.wordpress.com/2015/04/16/get-mailbox-folder-permissions-using-ews-multithreading/

.DESCRIPTION

Get folder mailbox permissions from EWS

.PARAMETER infile

Input CSV file with header comprising a mail field

.PARAMETER outfile

Output CSV file with PR_NT_SECURITY_DESCRIPTORS in hex

.PARAMETER server

Exchange CAS

.PARAMETER credential

Result of Get-Credential prompt (secure)

.PARAMETER username

Username of an Exchange Trusted Subsystem user (UPN or Netbios)

.PARAMETER password

Password (visible on command line, insecure)

.PARAMETER Threads

How many threads will be created. Default 5

.PARAMETER TrustAnySSL

Switch to trust any certificate.

.EXAMPLE

.\Get-MAPIFoldersPermissions.ps1 -infile '.\20170530_test.local\Ldap\TE_LDAP_obj.csv' -outfile '.\20170530_test.local\Ldap\TE_EWS_foldersd.csv' -server 192.168.67.128 -username TEST\exchadmin -password ...

.EXAMPLE

$exchcreds = Get-Credential
.\Get-MAPIFoldersPermissions.ps1 -infile '.\20170530_test.local\Ldap\TE_LDAP_obj.csv' -outfile '.\20170530_test.local\Ldap\TE_EWS_foldersd.csv' -server 192.168.67.128 -credential $exchcreds


.NOTES
#>

[CmdletBinding()]
Param (
  [parameter( Mandatory=$true, Position=0)]
  [string]$infile,

  [parameter( Mandatory=$true, Position=1)]
  [string]$outfile,

  [parameter( Mandatory=$false, Position=2)]
  [string]$server,
  
  [parameter( Mandatory=$false, Position=3)]
  [object]$credential,

  [parameter( Mandatory=$false, Position=3)]
  [string]$username,

  [parameter( Mandatory=$false, Position=4)]
  [string]$password,

  [parameter( Mandatory=$false, Position=5)]
  [ValidateRange(0,20)]
  [int]$Threads= '5',

  [parameter( Mandatory=$false, Position=6)]
  [switch]$TrustAnySSL=$true
)

Begin {

try {
  $EmailAddress = Import-Csv $infile -Encoding Unicode | Where-Object {$_.mail -ne "" -and $_.mail -notlike "SystemMailbox*" -and $_.mail -notlike "HealthMailbox*"} | Select mail 
}
catch {
  Error[0].Exception
}

#$EmailAddress
"mail,PR_NT_SECURITY_DESCRIPTOR" | Out-File -FilePath $outfile -Encoding Unicode

# Load Managed API dll
# Check for installed EWS on the system, else use redistributable from Github release, or fail
$EWSDLL = (($(Get-ItemProperty -ErrorAction SilentlyContinue -Path Registry::$(Get-ChildItem -ErrorAction SilentlyContinue -Path 'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Exchange\Web Services'|Sort-Object Name -Descending| Select-Object -First 1 -ExpandProperty Name)).'Install Directory') + "Microsoft.Exchange.WebServices.dll")

if (Test-Path $EWSDLL) {
  "EWS installation detected"
  Import-Module $EWSDLL
}
elseif (Test-Path $($PSScriptRoot + "\..\Bin\Microsoft.Exchange.WebServices.dll")) {
  Import-Module $($PSScriptRoot + "\..\Bin\Microsoft.Exchange.WebServices.dll")
}
else {
  "$(get-date -format yyyyMMddHHmmss):"
  "This script requires the EWS Managed API 1.2 or later."
  "Please download and install the current version of the EWS Managed API from"
  "http://go.microsoft.com/fwlink/?LinkId=255472"
  ""
  "Exiting Script."
  throw 'EWS unavailable on this system. https://go.microsoft.com/fwlink/?LinkId=255472'
  exit 1
}

## Set Exchange Version
$ExchangeVersion = [Microsoft.Exchange.WebServices.Data.ExchangeVersion]::Exchange2010_SP2

# Setup filters
# Search only for Inbox
$inboxFilter = new-object Microsoft.Exchange.WebServices.Data.SearchFilter+IsEqualTo([Microsoft.Exchange.WebServices.Data.FolderSchema]::DisplayName, "Inbox")
$sentmailFilter = new-object Microsoft.Exchange.WebServices.Data.SearchFilter+IsEqualTo([Microsoft.Exchange.WebServices.Data.FolderSchema]::DisplayName, "Sent Items")

$boiteString = "Bo$([char]0x00ee)te de r$([char]0x00e9)ception"
$elementsString = "$([char]0x00c9)l$([char]0x00e9)ments envoy$([char]0x00e9)s"
$boiteFilter = new-object Microsoft.Exchange.WebServices.Data.SearchFilter+IsEqualTo([Microsoft.Exchange.WebServices.Data.FolderSchema]::DisplayName, $boiteString)
$elementsFilter = new-object Microsoft.Exchange.WebServices.Data.SearchFilter+IsEqualTo([Microsoft.Exchange.WebServices.Data.FolderSchema]::DisplayName, $elementsString)

$compoundFilter = New-Object Microsoft.Exchange.WebServices.Data.SearchFilter+SearchFilterCollection([Microsoft.Exchange.WebServices.Data.LogicalOperator]::Or)
$compoundFilter.add($inboxFilter)
$compoundFilter.add($sentmailFilter)
$compoundFilter.add($boiteFilter)
$compoundFilter.add($elementsFilter)

# Create Exchange Service Object
$service = New-Object Microsoft.Exchange.WebServices.Data.ExchangeService($ExchangeVersion)
# Set Credentials to use two options are availible Option1 to use explict credentials or Option 2 use the Default (logged On) credentials
If ($username -and $password) {
  #Credentials Option 1 using UPN for the windows Account
  $creds = New-Object System.Net.NetworkCredential($username,$password)
  $service.Credentials = $creds
}
ElseIf ($credential) {
  "Using exchcreds"
  $service.Credentials = $credential.GetNetworkCredential()
}
Else {
  #Credentials Option 2
  $service.UseDefaultCredentials = $true
}

# Choose to ignore any SSL Warning issues caused by Self Signed Certificates
If ($TrustAnySSL) {
  # Code From http://poshcode.org/624
  # Create a compilation environment
  $Provider=New-Object Microsoft.CSharp.CSharpCodeProvider
  $Compiler=$Provider.CreateCompiler()
  $Params=New-Object System.CodeDom.Compiler.CompilerParameters
  $Params.GenerateExecutable=$False
  $Params.GenerateInMemory=$True
  $Params.IncludeDebugInformation=$False
  $Params.ReferencedAssemblies.Add("System.DLL") | Out-Null

  $TASource=@'
    namespace Local.ToolkitExtensions.Net.CertificatePolicy {
      public class TrustAll : System.Net.ICertificatePolicy {
        public TrustAll() {
        }
        public bool CheckValidationResult(System.Net.ServicePoint sp,
        System.Security.Cryptography.X509Certificates.X509Certificate cert,
        System.Net.WebRequest req, int problem) {
          return true;
        }
      }
    }
'@

  $TAResults=$Provider.CompileAssemblyFromSource($Params,$TASource)
  $TAAssembly=$TAResults.CompiledAssembly

  ## We now create an instance of the TrustAll and attach it to the ServicePointManager
  $TrustAll=$TAAssembly.CreateInstance("Local.ToolkitExtensions.Net.CertificatePolicy.TrustAll")
  [System.Net.ServicePointManager]::CertificatePolicy=$TrustAll

  ## end code from http://poshcode.org/624
}

# Set the URL of the CAS (Client Access server)
$uri=[system.URI] "https://$server/ews/exchange.asmx"
$service.Url = $uri

"Test EWS Connectivity"
$testFolder = $null
$testFolderId = new-object Microsoft.Exchange.WebServices.Data.FolderId([Microsoft.Exchange.WebServices.Data.WellKnownFolderName]::"Root")
$testFolderView = New-Object Microsoft.Exchange.WebServices.Data.FolderView(1)
$service
$service.Credentials
try {
  $service.FindFolders($testFolderId, $testFolderView) | Out-Null
}
catch {
  $Error[0].Exception
  exit 1
}
"OK EWS Connectivity"
}

Process {

###################################################
# Helper functions
###################################################

function ForEach-Parallel {
  param(
    [Parameter(Mandatory=$true,position=0)]
    [System.Management.Automation.ScriptBlock] $ScriptBlock,
    [Parameter(Mandatory=$true,ValueFromPipeline=$true)]
    [PSObject]$InputObject,
    [Parameter(Mandatory=$false)]
    [int]$MaxThreads=5
  )
  BEGIN {
    $iss = [system.management.automation.runspaces.initialsessionstate]::CreateDefault()
    $pool = [Runspacefactory]::CreateRunspacePool(1, $maxthreads, $iss, $host)
    $pool.open()
    $threads = @()
    $ScriptBlock = $ExecutionContext.InvokeCommand.NewScriptBlock("param(`$_)`r`n" + $Scriptblock.ToString())
  }
  
  PROCESS {
    $powershell = [powershell]::Create().addscript($scriptblock).addargument($InputObject)
    $powershell.runspacepool=$pool
    $threads+= @{
      instance = $powershell
      handle = $powershell.begininvoke()
    }
  }
  
  END {
    $notdone = $true
    while ($notdone) {
      $notdone = $false
      for ($i=0; $i -lt $threads.count; $i++) {
        $thread = $threads[$i]
        if ($thread) {
          if ($thread.handle.iscompleted) {
            $thread.instance.endinvoke($thread.handle)
            $thread.instance.dispose()
            $threads[$i] = $null
            $boxCounter++
            Write-Progress -Activity "Processing mailboxes..." -Status "Current mailbox: $boxCounter / $boxTotal"
          }
          else {
            $notdone = $true
          }
        }
      }
    }
  }
}

###################################
# End of helper functions
###################################
$mutexName = "outfileMutex-" + [guid]::NewGuid().Guid
$outfilePath = Resolve-Path $outfile

try{
  $threadParams = @()
  $EmailAddress | ForEach {
  $obj = $null
  $obj = New-Object System.Object
  $obj | Add-Member -type NoteProperty -Name mail -Value $_.mail
  $obj | Add-Member -type NoteProperty -Name Service -Value $Service
  $obj | Add-Member -type NoteProperty -Name compoundfilter -Value $compoundFilter
  $obj | Add-Member -type NoteProperty -Name outfile -Value $outfilePath
  $obj | Add-Member -type NoteProperty -Name mutexName -Value $mutexName

  $threadParams += $obj
}

#
# "Processing " + $EmailAddress.count + " emails"
#
$boxTotal = $EmailAddress.count
$boxCounter = 0

$threadParams | ForEach-Parallel  -Maxthread $Threads {
$MailboxName = $_.mail
$outfile = $_.outfile
$mtx = New-Object System.Threading.Mutex($false, $_.mutexName)

function BinToHex {
param(
  [Parameter(
    Position=0,
    Mandatory=$true,
    ValueFromPipeline=$true)
  ]
  [Byte[]]$Bin
  )
# assume pipeline input if we don't have an array (surely there must be a better way)
if ($bin.Length -eq 1) { $bin = @($input) }
$return = -join ($Bin |  foreach { "{0:X2}" -f $_ })
Write-Output $return
}

$rootFolderId = new-object Microsoft.Exchange.WebServices.Data.FolderId([Microsoft.Exchange.WebServices.Data.WellKnownFolderName]::"MsgFolderRoot",$MailboxName)

#Define Extended properties
$PR_FOLDER_TYPE = new-object Microsoft.Exchange.WebServices.Data.ExtendedPropertyDefinition(13825,[Microsoft.Exchange.WebServices.Data.MapiPropertyType]::Integer);
$folderidcnt = $rootFolderId

#Define the FolderView used for Export should not be any larger then 1000 folders due to throttling
$fvFolderView =  New-Object Microsoft.Exchange.WebServices.Data.FolderView(1000)

#Deep Transval will ensure all folders in the search path are returned
$fvFolderView.Traversal = [Microsoft.Exchange.WebServices.Data.FolderTraversal]::Deep;
$psPropertySet = new-object Microsoft.Exchange.WebServices.Data.PropertySet([Microsoft.Exchange.WebServices.Data.BasePropertySet]::FirstClassProperties)
$PR_Folder_Path = new-object Microsoft.Exchange.WebServices.Data.ExtendedPropertyDefinition(26293, [Microsoft.Exchange.WebServices.Data.MapiPropertyType]::String);

#Add Properties to the Property Set
$psPropertySet.Add($PR_Folder_Path);
$fvFolderView.PropertySet = $psPropertySet;

$fiResult = $null
try {
  $fiResult = $_.Service.FindFolders($folderidcnt,$_.compoundFilter,$fvFolderView)
}
catch {
  continue
}

if($fiResult.TotalCount -eq 0) {
  continue
}

# Add Properties for the Folder Property Set
$PR_NT_SECURITY_DESCRIPTOR = new-object Microsoft.Exchange.WebServices.Data.ExtendedPropertyDefinition(0x0E27, [Microsoft.Exchange.WebServices.Data.MapiPropertyType]::Binary);
$folderPropset = new-object Microsoft.Exchange.WebServices.Data.PropertySet([Microsoft.Exchange.WebServices.Data.BasePropertySet]::FirstClassProperties)
$folderPropset.Add($PR_NT_SECURITY_DESCRIPTOR)
$folderPropset.Add($PR_Folder_Path)

ForEach ($Folder in $fiResult) {
  If (($Folder.Displayname -ne 'System') -and ($Folder.Displayname -ne 'Audits')) {
    # Write-Output "Working on $($Folder.Displayname)"
    # Load Properties
    $Folder.Load($folderPropset)

    # get PR_NT_SECURITY_DESCRIPTOR
    $sd = BinToHex -Bin ($Folder.ExtendedProperties[0].Value)
    $mtx.WaitOne() | Out-Null
    $MailboxName + "," + $sd.Substring(16) | Out-File -FilePath $outfile -Append -Encoding Unicode
    $mtx.ReleaseMutex()
  }
  #end loop of folders
}
}
}
catch{
  $Error[0].Exception
}
}
