function Get-ADCPDump {
    <#
    .SYNOPSIS

        Dump Data with ADCP

    .DESCRIPTION

        .

    .PARAMETER outputtDir

        Output Directory

    .PARAMETER logLevel

        Tool verbosity

    .PARAMETER domainController

        Specify which Domain Controller to request data from.

    .PARAMETER ldapport

        Specify the LDAP port to query. Useful for DSAMAIN mounted ntds.dit.

    .PARAMETER domaindnsName

        Specify the domain DNS name.

    .PARAMETER sysvolPath

        Specify an optional alternative sysvol path.

    .PARAMETER Credential

        Credentials for Active Directory connection (PSCredential)

    .PARAMETER useBackupPriv

        Use backup privilege for sysvol operations (require Administrator privileges)

    .PARAMETER ldapOnly

        Only performs LDAP operations

    .PARAMETER sysvolOnly

        Only performs Sysvol operations

    .PARAMETER exchangeServer

        Exchange server with EWS API

    .PARAMETER ExchangeCredential

        Exchange Trusted Subsystem member NOT in DA/EA/Org Mgmt

    .PARAMETER forceOverwrite
        Discard existing results

    .EXAMPLE

        C:\ADCP> .\Dump\Dump.ps1 <folder.

    #>

    Param(
        [Parameter(Mandatory=$True, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$outputDir = $null,
        [string]$logLevel = 'INFO',

        [Parameter(Mandatory=$True, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$domainController = $null,
        [int]$ldapPort = 389,
        [Parameter(Mandatory=$True, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$domainDnsName = $null,
        [string]$sysvolPath = $null,
    
        [ValidateNotNull()]
        [System.Management.Automation.PSCredential]
        [System.Management.Automation.Credential()]
        $Credential = [System.Management.Automation.PSCredential]::Empty,
    
        [switch]$useBackupPriv = $false,
    
        [switch]$ldapOnly = $false,
        [switch]$sysvolOnly = $false,

        [string]$exchangeServer = $null,
        
        [ValidateNotNull()]
        [System.Management.Automation.PSCredential]
        [System.Management.Automation.Credential()]
        $ExchangeCredential = [System.Management.Automation.PSCredential]::Empty,

        [switch]$forceOverwrite = $false
    )

    $globalTimer = $null
    $globalLogFile = $null

    Function Write-Output-And-Global-Log([string]$str)
    {
        if(!$generateCmdOnly) {
            Write-Output $str
            Add-Content $globalLogFile "[$($globalTimer.Elapsed)]$str"
        }
    }

    if (-not (Test-Path -Path $outputDir)) {
        New-item -ItemType Directory -Path $outputDir | Out-Null
    }

    $date =  Get-Date -Format yyyyMMdd

    $outputParentDir = $outputDir
    $outputDir = Join-Path -Path $outputDir -ChildPath "\$date`_$domainDnsName"
    $filesPrefix = $domainDnsName.Substring(0,2).ToUpper()

    $directories = (
        "$outputDir",
        "$outputDir\Ldap",
        "$outputDir\Logs",
        "$outputDir\Relations"
    )
    Foreach ($dir in $directories) {
        if (!(Test-Path -Path $dir)) {
            New-Item -ItemType directory -Path $dir | Out-Null
            # No native PS equivalent
            compact /C $dir | Out-Null
        }
    }
    $globalLogFile = "$outputDir\Logs\$filesPrefix.global.log"
    if ((Test-Path -Path $globalLogFile) -and $forceOverwrite) {
        Clear-Content $globalLogFile
    } elseif (Test-Path -Path $globalLogFile) {
        Write-Error -Message "Previous log file detected, use -forceOverwrite to start over"
        break
    }

    # Process
    $globalTimer = [Diagnostics.Stopwatch]::StartNew()

    if (!$sysvolPath) {
        $sysvolPath = '\\'+$domainController+'\SYSVOL\'+$domainDnsName+'\Policies'
        Write-Output-And-Global-Log "[+] Using default Sysvol path $sysvolPath"
    }

    Write-Output-And-Global-Log "Current arguments:"
    foreach ($key in $MyInvocation.BoundParameters.keys) {
        $value = (get-variable $key).Value 
        Write-Output-And-Global-Log "$key -> $value"
    }
    Write-Output-And-Global-Log "[+] Starting"
    if($Credential -ne [System.Management.Automation.PSCredential]::Empty) {
        $username = $Credential.username
        Write-Output-And-Global-Log "[+] Using explicit authentication with username '$username'"
        $password = $Credential.GetNetworkCredential().Password
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
    if($exchangeServer.IsPresent) {
        Write-Output-And-Global-Log "[+] Dumping EXCHANGE data`n"
    }

    $dumpLdap = $ldapOnly.IsPresent -or (!$ldapOnly.IsPresent -and !$sysvolOnly.IsPresent)
    $dumpSysvol = $sysvolOnly.IsPresent -or (!$ldapOnly.IsPresent -and !$sysvolOnly.IsPresent)

    # Common params for command lines
    $optionalParams = (
        ($ldapPort,         "-n '$ldapPort'"),
        ($Credential.UserName,       "-l '$username' -p '$password'"),
        ($domainDnsName,    "-d '$domainDnsName'")
        )

    if ($dumpLdap) {
        # Directory Crawler dumps Ldap Data
        Execute-Cmd-Wrapper -optionalParams $optionalParams -cmd @"
.\Bin\directorycrawler.exe
-w '$logLevel'
-f '$outputDir\Logs\$filesPrefix.dircrwl.log'
-j '.\Bin\ADng_ADCP.json'
-o '$outputParentDir'
-s '$domainController'
-c '$filesPrefix'
"@

        # Exchange requires LDAP DATA (LDAP_obj.csv)
        if ($ExchangeServer -and $ExchangeCredential) {
            # Inbox Folder MAPI SD
            Execute-Cmd-Wrapper -cmd @"
.\Utils\Get-MAPIFoldersPermissions.ps1
-infile '$outputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
-outfile '$outputDir\Ldap\$($filesPrefix)_EWS_foldersd.csv'
-server $ExchangeServer
-credential `$ExchangeCredential
"@
        }
    }

    if($dumpSysvol) {
        Write-Output-And-Global-Log "[+] Mapping SYSVOL $($sysvolPath)"
        for($j=67; Get-PSDrive ($driveName=[char]$j++) -erroraction 'silentlycontinue'){}
        if($Credential -ne [System.Management.Automation.PSCredential]::Empty) {
            New-PSDrive -PSProvider FileSystem -Root $sysvolPath -Name $driveName -Credential $Credential -Scope Global -Persist
        }
        else {
            New-PSDrive -PSProvider FileSystem -Root $sysvolPath -Name $driveName -Scope Global -Persist
        }
        $sysvolDrive = $driveName + ':'
        if (Test-Path $sysvolDrive) {
            Write-Output-And-Global-Log "[+] Mapping successful"
            Write-Output-And-Global-Log "[+] Dumping SYSVOL permissions"
        }
        else {
            Write-Output-And-Global-Log "[-] Mapping FAILED"
            Write-Output-And-Global-Log "[-] If the Error is Access denied, in HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows\NetworkProvider\HardenedPaths , add a REG_SZ value '\\*\SYSVOL' with RequiredMutualAuthentication=0"
            Write-Output-And-Global-Log "[-] Don't forget to remove it!"
            $globalTimer.Stop()
            Write-Output-And-Global-Log "[+] Done. Total time: $($globalTimer.Elapsed)`n"
        }
        Get-ChildItem -Path $sysvolDrive -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
            $file = $_
            try {
                $rsd = $file.GetAccessControl().GetSecurityDescriptorBinaryForm()
                New-Object -TypeName PSObject -Property @{UNC=($file.FullName -Replace "^$sysvolDrive","$sysvolPath" ); nTSecurityDescriptor=($rsd|ForEach-Object ToString X2) -join ''}             
            }
            catch {
                New-Object -TypeName PSObject -Property @{UNC=$file.FullName -Replace "^$sysvolDrive","$sysvolPath"; nTSecurityDescriptor="ERROR"}
            }
        } | Export-Csv -Encoding Unicode -NoTypeInformation $outputDir\Ldap\$($filesPrefix)_SYSVOL_acl.csv
        Remove-PSDrive -Name $driveName
    }

$obj = New-Object -TypeName PSObject
$obj | Add-Member -MemberType NoteProperty -name 'inputDir' -value $outputDir -PassThru |
       Add-member -MemberType NoteProperty -name 'domainDnsName' -value $domainDnsName -PassThru |
       Add-Member -MemberType NoteProperty -name 'logLevel' -value $logLevel -PassThru
}
