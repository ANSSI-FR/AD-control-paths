#
# Script parameters & variables
# 
Param(
  [string]$outputDir = $null,
  [string]$logLevel = 'INFO',

  [string]$domainController = $null,
  [int]$ldapPort = $null,
  [string]$domainDnsName = $null,
  [string]$sysvolPath = $null,
  
  [string]$user = $null,
  [string]$password = $null,
  
  [switch]$useBackupPriv = $false,
  
  [switch]$ldapOnly = $false,
  [switch]$sysvolOnly = $false,
  
  [string]$exchangeServer = $null,
  [string]$exchangeUsername = $null,
  [string]$exchangePassword = $null,

  [switch]$fromExistingDumps = $false,
  [switch]$forceOverwrite = $false,
  [switch]$resume = $false,
  
  [switch]$help = $false,
  [switch]$generateCmdOnly = $false
)

$globalTimer = $null
$globalLogFile = $null
$dumpLdap = $ldapOnly.IsPresent -or (!$ldapOnly.IsPresent -and !$sysvolOnly.IsPresent)
$dumpSysvol = $sysvolOnly.IsPresent -or (!$ldapOnly.IsPresent -and !$sysvolOnly.IsPresent)
$dumpExchange = $($exchangeServer -ne "")
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
  
  Write-Output "- Required parameters:"
  Write-Output "`t-outputDir <DIR>                : output directory"
  Write-Output "`t-domainController <DC>          : ip or hostname of the DC to query"
  Write-Output "`t-domainDnsName <DNSNAME>        : dns name of the domain (ex: mydomain.local)"
  
  Write-Output "- Optional parameters:"
  Write-Output "`t-help                           : show this help"

  Write-Output "`t-sysvolPath <PATH>              : path of the 'Policies' folder of the sysvol (default: use target DC)"
  Write-Output "`t-user <USRNAME> -password <PWD> : username and password to use for explicit authentication"
  Write-Output "`t-logLevel <LVL>                 : log level, possibles values are ALL,DBG,INFO(default),WARN,ERR,SUCC,NONE"
  Write-Output "`t-ldapPort <PORTNUM>             : ldap port to use (default is 389)"
  
  Write-Output "`t-exchangeServer <EXCHSRV>       : dump Exchange data (needs EWS managed API) "
  Write-Output "`t-exchangeUser <EXCHUSR>         : Exchange Trusted Subsystem member NOT in DA/EA/Org Mgmt"
  Write-Output "`t-exchangePassword <EXCHPWD>     : password of exchangeUser"
  
  Write-Output "`t-useBackupPriv                  : use backup privilege to access -sysvolPath"
  Write-Output "`t-ldapOnly                       : only dump data from the LDAP directory"
  Write-Output "`t-sysvolOnly                     : only dump data from the sysvol"
  Write-Output "`t-fromExistingDumps              : use previous ldap files in the same day/target folder"
  Write-Output "`t-resume                         : resume from the first non-successful command in the same day/target folder"
  Write-Output "`t-forceOverwrite                 : overwrite any previous dump files from same day/target folder"
  Write-Output "`t-generateCmdOnly                : generate a list of commands to dump data, instead of executing them"

  Break
}

if($help -or ($args -gt 0)) {
  Usage
}
if(!$outputDir) {
  Usage "-outputDir parameter is required."
}
if(!$domainController) {
  Usage "-domainController parameter is required."
}
if(!$domainDnsName) {
  Usage "-domainDnsName parameter is required."
}
if([bool]$user -bXor [bool]$password) {
  Usage "-user and -password must both be specified to use explicit authentication"
}
if($ldapPort -lt 0) {
  Usage "-ldapPort must be > 0"
}
if($ldapOnly.IsPresent -and $sysvolOnly.IsPresent) {
  Usage "-ldapOnly and -sysvolOnly cannot be used at the same time"
}
if($forceOverwrite.IsPresent -and $resume.IsPresent) {
  Usage "-forceOverwrite and -resume cannot be used at the same time"
}
if(([bool]$exchangeUsername -bXor [bool]$exchangeServer) -bOr ([bool]$exchangeUsername -bXor [bool]$exchangePassword)) {
    Usage "-exchangeUser and -exchangePassword must both be specified along with exchangeServer"
}

#
# Functions
#
Function Write-Output-And-Global-Log([string]$str)
{
  if(!$generateCmdOnly) {
    Write-Output $str
    Add-Content $globalLogFile "[$($globalTimer.Elapsed)] $str"
  }
}
 
Function Execute-Cmd-Wrapper([string]$cmd, [array]$optionalParams, [bool]$maxRetVal=0)
{
  Foreach ($param in $optionalParams) {
    if($param) {
      $val = $param[0]
      $str = $param[1]
      if( $val ) { # ! null/empty/whitespaces/false...
        $cmd += " $str"
      }
    }
  }
  $cmd = $cmd -replace '\s+', ' '
  $error = $null
  $timer = $null
  
  if($generateCmdOnly) {
    Write-Output $cmd
  }
  elseif($resume -and (Select-String -Path $checkpointFile -Pattern $cmd -Context 0,2 -SimpleMatch | Out-String | Select-String -Pattern "Return : OK")) {
    Write-Output-And-Global-Log "********************"
    Write-Output-And-Global-Log "* Command: $cmd"
    Write-Output-And-Global-Log "* Command previously successful in checkpoint, skipping."
    Write-Output-And-Global-Log "* Return : OK - 0"
    Write-Output-And-Global-Log "********************`n"
  }
  else {
    Try {
      Write-Output-And-Global-Log "********************"
      Write-Output-And-Global-Log "* Command: $cmd"
      Write-Output "*"
      $timer = [Diagnostics.Stopwatch]::StartNew()
      Invoke-Expression $cmd
      if(($LASTEXITCODE -lt 0) -or ($LASTEXITCODE -gt $maxRetVal)) {
        throw "return code out-of-range ($LASTEXITCODE)"
      }
    } Catch {
      $error = $_.Exception.Message
      Break
    } Finally {
      $timer.Stop()
      Write-Output "*"
      Write-Output-And-Global-Log "* Time   : $($timer.Elapsed)"
      if($error) {
        Write-Output-And-Global-Log "* Return : FAIL - $error"
        $globalTimer.Stop()
        Write-Output-And-Global-Log "[+] Done. Total time: $($globalTimer.Elapsed)`n"
        exit
      } else {
        Write-Output-And-Global-Log "* Return : OK - $LASTEXITCODE"
      }
      Write-Output-And-Global-Log "********************`n"
    }
  }
}

# 
# Start
# 
$globalTimer = [Diagnostics.Stopwatch]::StartNew()

# 
# Creating output directories
#
#   $outputDir\
#     |- Ldap
#     |- Logs
#     \- Relations
#
$outputDirParent = $outputDir
$outputDir += "\$date`_$domainDnsName"
$filesPrefix = $domainDnsName.Substring(0,2).ToUpper()
$directories = (
  "$outputDir",
  "$outputDir\Ldap",
  "$outputDir\Logs",
  "$outputDir\Relations"
)

if (!$generateCmdOnly) {
  Foreach($dir in $directories) {
    if(!(Test-Path -Path $dir)) {
      New-Item -ItemType directory -Path $dir | Out-Null
      # No native PS equivalent
      compact /C $dir | Out-Null
    }
  }

  $globalLogFile = "$outputDir\Logs\$filesPrefix.global.log"
  $checkpointFile = "$outputDir\Logs\$filesPrefix.checkpoint.log"
  if((Test-Path -Path $globalLogFile) -and $forceOverwrite) {
    Clear-Content $globalLogFile
  }
  elseif((Test-Path -Path $globalLogFile) -and !$forceOverwrite -and !$resume) {
    Usage "Previous log file detected, use -forceOverwrite to start over or -resume"
  }
  elseif((Test-Path -Path $globalLogFile) -and $resume) {
    Copy-Item $globalLogFile $checkpointFile
    Clear-Content $globalLogFile
  }
}

if (!$sysvolPath) {
  $sysvolPath = '\\'+$domainController+'\SYSVOL\'+$domainDnsName+'\Policies'
  Write-Output-And-Global-Log "[+] Using default Sysvol path $sysvolPath`n"
}

Write-Output-And-Global-Log "Current arguments:"
foreach ($key in $MyInvocation.BoundParameters.keys)
{
  $value = (get-variable $key).Value 
  Write-Output-And-Global-Log "$key -> $value"
}
Write-Output-And-Global-Log "[+] Starting"
if($user) {
  Write-Output-And-Global-Log "[+] Using explicit authentication with username '$user'"
} else {
  Write-Output-And-Global-Log "[+] Using implicit authentication"
}
if($ldapOnly.IsPresent) {
  Write-Output-And-Global-Log "[+] Dumping LDAP data only`n"
} elseif($sysvolOnly.IsPresent) {
  Write-Output-And-Global-Log "[+] Dumping SYSVOL data only`n"
} else {
  Write-Output-And-Global-Log "[+] Dumping LDAP and SYSVOL data`n"
}
if($dumpExchange.IsPresent) {
  Write-Output-And-Global-Log "[+] Dumping EXCHANGE data`n"
}
if($fromExistingDumps.IsPresent) {
  Write-Output-And-Global-Log "[+] Working from existing dump files`n"
}

if($dumpLdap -and !$fromExistingDumps.IsPresent) {
# 
# LDAP data
# 
$optionalParams = (
  ($ldapPort,         "-n '$ldapPort'"),
  ($user,             "-l '$user' -p '$password'"),
  ($domainDnsName,    "-d '$domainDnsName'")
)

# Dump
Execute-Cmd-Wrapper -optionalParams $optionalParams -cmd @"
.\Bin\directorycrawler.exe
-w '$logLevel'
-f '$outputDir\Logs\$filesPrefix.dircrwl.log'
-j '.\Bin\ADng_ADCP.json'
-o '$outputDirParent'
-s '$domainController'
-c '$filesPrefix'
"@
}

if($dumpLdap) {
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Container.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.container.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.container.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Gplink.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.gplink.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.gplink.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Group.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.group.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.group.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Sd.exe
  -D '$logLevel'
  -L '$outputDir\logs\$filesPrefix.control.ad.sd.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -A '$outputDir\Ldap\$($filesPrefix)_LDAP_ace.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.sd.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.PrimaryGroup.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.primarygroup.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.primarygroup.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.SidHistory.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.sidhistory.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.sidhistory.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Rodc.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.sidhistory.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.rodc.csv'
  -Y '$outputDir\Relations\$filesPrefix.control.ad.rodc.deny.csv'
"@

Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Deleg.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.ad.deleg.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.ad.deleg.csv'
"@

# Filter
Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$outputDir\Logs\$filesPrefix.acefilter.ldap.msr.log'
  --importer='LdapDump'
  --writer='MasterSlaveRelation'
  --filters='Inherited,ObjectType,ControlAd'
  --
  msrout='$outputDir\Relations\$filesPrefix.acefilter.ldap.msr.csv'
  ldpobj='$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$outputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$outputDir\Ldap\$($filesPrefix)_LDAP_ace.csv'
"@

}

#
# EXCHANGE data
#
if($dumpExchange) {
# DB to contained mailboxes
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Exch.Db.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.exch.db.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.exch.db.csv'
"@

# RBAC Principals to Roles
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Exch.Role.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.exch.role.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.exch.role.csv'
"@

# RBAC Roles to RoleEntries
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Exch.RoleEntry.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.exch.roleentry.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.exch.roleentry.csv'
"@

# OWNER MBX SD
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Mbx.Sd.exe
  -D '$logLevel'
  -L '$outputDir\logs\$filesPrefix.control.mbx.sd.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -A '$outputDir\Ldap\$($filesPrefix)_LDAP_mbxsd.csv'
  -O '$outputDir\Relations\$filesPrefix.control.exch.mbxsd.csv'
"@

# Filter DB SD
Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$outputDir\Logs\$filesPrefix.acefilter.exch.db.log'
  --importer='LdapDump'
  --writer='MasterSlaveRelation'
  --filters='InheritedObjectType,ControlMbx'
  --
  msrout='$outputDir\Relations\$filesPrefix.acefilter.exch.db.csv'
  ldpobj='$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$outputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$outputDir\Ldap\$($filesPrefix)_LDAP_exchdb.csv'
"@

# Filter MBX SD
Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$outputDir\Logs\$filesPrefix.acefilter.mbx.sd.log'
  --importer='LdapDump'
  --writer='MasterMailboxRelation'
  --filters='Inherited,ControlMbx'
  --
  mmrout='$outputDir\Relations\$filesPrefix.acefilter.mbx.sd.csv'
  ldpobj='$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$outputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$outputDir\Ldap\$($filesPrefix)_LDAP_mbxsd.csv'
"@

# Inbox Folder MAPI SD
Execute-Cmd-Wrapper -cmd @"
.\Utils\Get-MAPIFoldersPermissions.ps1
  -infile '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'   
  -outfile '$outputDir\Ldap\$($filesPrefix)_EWS_foldersd.csv'
  -server $ExchangeServer
  -username $ExchangeUsername
  -password $ExchangePassword
"@

# Filter MAPI Folders SD
Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$outputDir\Logs\$filesPrefix.acefilter.folder.sd.log'
  --importer='LdapDump'
  --writer='MasterSlaveRelation'
  --filters='MapiFolder'
  --
  msrout='$outputDir\Relations\$filesPrefix.acefilter.folder.sd.csv'
  ldpobj='$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$outputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$outputDir\Ldap\$($filesPrefix)_EWS_foldersd.csv'
"@

}

# 
# SYSVOL data
# 
if($dumpSysvol) {

#
# net use sysvol in case of explicit authentication
#
if([bool]$user -bAnd [bool]$password -bAnd ![bool]$ldapOnly) {
  Write-Output-And-Global-Log "[+] Mapping SYSVOL $($sysvolPath)"
  $secstr = convertto-securestring -String $password -AsPlainText -Force
  $cred = new-object -typename System.Management.Automation.PSCredential -argumentlist $user, $secstr
  for($j=67;gdr($driveName=[char]$j++) -erroraction 'silentlycontinue'){}
  New-PSDrive -PSProvider FileSystem -Root $sysvolPath -Name $driveName -Credential $cred -Scope Global -Persist
  $sysvolPath = $driveName + ':'
  if (Test-Path $sysvolPath) {
    Write-Output-And-Global-Log "[+] Mapping successful"
  }
  else {
    Write-Output-And-Global-Log "[-] Mapping FAILED"
    $globalTimer.Stop()
    Write-Output-And-Global-Log "[+] Done. Total time: $($globalTimer.Elapsed)`n"
    exit
  }
}

# GPO Owners
$optionalParams += , ($useBackupPriv, "-B")
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Sysvol.Sd.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.control.sysvol.sd.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$outputDir\Relations\$filesPrefix.control.sysvol.sd.csv'
  -S '$sysvolPath'
"@

# GPO files ACE filtering
$optionalParams = (,($useBackupPriv, "usebackpriv=1"))
Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$outputDir\logs\$filesPrefix.acefilter.sysvol.msr.log'
  --importers='Sysvol,LdapDump'
  --writer='MasterSlaveRelation'
  --filters='Inherited,ControlFs'
  --
  msrout='$outputDir\Relations\$filesPrefix.acefilter.sysvol.msr.csv'
  ldpobj='$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$outputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  sysvol='$sysvolPath'
"@

#
# Deleting net use sysvol in case of explicit authentication
#
if([bool]$user -bAnd [bool]$password -bAnd ![bool]$ldapOnly) {
  Write-Output-And-Global-Log "[+] Unmapping SYSVOL"
  Remove-PSDrive -Name $driveName
}

}

#
# Make all nodes from LDAP and relations
#
Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.MakeAllNodes.exe
  -D '$logLevel'
  -L '$outputDir\Logs\$filesPrefix.makeallnodes.log'
  -I '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -A '$((dir $outputDir\Relations\*.csv -exclude *.deny.csv) -join ',')'
  -O '$outputDir\Ldap\all_nodes.csv'
"@

# 
# End
# 
$globalTimer.Stop()
Write-Output-And-Global-Log "[+] Done. Total time: $($globalTimer.Elapsed)`n"
