function Prepare-ADCPDump {
    <#
    .SYNOPSIS

        Preprocess ADCP Results


    .DESCRIPTION

        .

    .PARAMETER inputDir

        Output Directory of Invoke-ADCPDump

    .EXAMPLE

        x
    #>

    Param(
        [Parameter(Mandatory=$True, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$inputDir = $null,
        [string]$logLevel = 'INFO',
        [Parameter(Mandatory=$True, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$domainDnsName = $null
    )

    if (-not (Test-Path -Path $inputDir)) {
        Write-Error "Invalid inputDir"
        Return
    }

    $globalTimer = $null
    $globalLogFile = $null
    Function Write-Output-And-Global-Log([string]$str)
    {
        if(!$generateCmdOnly) {
            Write-Output $str
            Add-Content $globalLogFile "[$($globalTimer.Elapsed)]$str"
        }
    }

    $filesPrefix = $domainDnsName.Substring(0,2).ToUpper()

    $globalLogFile = "$inputDir\Logs\$filesPrefix.prepare.log"
    if ((Test-Path -Path $globalLogFile)) {
        Clear-Content $globalLogFile
    }

    # Process
    $globalTimer = [Diagnostics.Stopwatch]::StartNew()

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Container.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.container.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.container.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Gplink.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.gplink.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.gplink.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Group.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.group.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.group.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Sd.exe
  -D '$logLevel'
  -L '$inputDir\logs\$filesPrefix.control.ad.sd.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -A '$inputDir\Ldap\$($filesPrefix)_LDAP_ace.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.sd.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.PrimaryGroup.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.primarygroup.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.primarygroup.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.SidHistory.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.sidhistory.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.sidhistory.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Rodc.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.rodc.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.rodc.csv'
  -Y '$inputDir\Relations\$filesPrefix.control.ad.rodc.deny.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Ad.Deleg.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.ad.deleg.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.ad.deleg.csv'
"@

    Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$inputDir\Logs\$filesPrefix.acefilter.ldap.msr.log'
  --importer='LdapDump'
  --writer='MasterSlaveRelation'
  --filters='Inherited,ObjectType,ControlAd'
  --
  msrout='$inputDir\Relations\$filesPrefix.acefilter.ldap.msr.csv'
  ldpobj='$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$inputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$inputDir\Ldap\$($filesPrefix)_LDAP_ace.csv'
"@
    
    If (Test-Path -Path '$inputDir\Ldap\$($filesPrefix)_EWS_foldersd.csv') {
# OWNER DB
    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Exch.Db.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.exch.db.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.exch.db.csv'
"@

# RBAC Principals to Roles
    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Exch.Role.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.exch.role.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.exch.role.csv'
"@

# RBAC Roles to RoleEntries
    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Exch.RoleEntry.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.control.exch.roleentry.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -O '$inputDir\Relations\$filesPrefix.control.exch.roleentry.csv'
"@

# OWNER MBX SD
    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.Mbx.Sd.exe
  -D '$logLevel'
  -L '$inputDir\logs\$filesPrefix.control.mbx.sd.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -A '$inputDir\Ldap\$($filesPrefix)_LDAP_mbxsd.csv'
  -O '$inputDir\Relations\$filesPrefix.control.exch.mbxsd.csv'
"@

# Filter DB SD
    Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$inputDir\Logs\$filesPrefix.acefilter.exch.db.log'
  --importer='LdapDump'
  --writer='MasterSlaveRelation'
  --filters='InheritedObjectType,ControlMbx'
  --
  msrout='$inputDir\Relations\$filesPrefix.acefilter.exch.db.csv'
  ldpobj='$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$inputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$inputDir\Ldap\$($filesPrefix)_LDAP_exchdb.csv'
"@

# Filter MBX SD
    Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$inputDir\Logs\$filesPrefix.acefilter.mbx.sd.log'
  --importer='LdapDump'
  --writer='MasterMailboxRelation'
  --filters='Inherited,ControlMbx'
  --
  mmrout='$inputDir\Relations\$filesPrefix.acefilter.mbx.sd.csv'
  ldpobj='$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$inputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$inputDir\Ldap\$($filesPrefix)_LDAP_mbxsd.csv'
"@

# Filter MAPI Folders SD
    Execute-Cmd-Wrapper -cmd @"
.\Bin\AceFilter.exe
  --loglvl='$logLevel'
  --logfile='$inputDir\Logs\$filesPrefix.acefilter.folder.sd.log'
  --importer='LdapDump'
  --writer='MasterSlaveRelation'
  --filters='MapiFolder'
  --
  msrout='$inputDir\Relations\$filesPrefix.acefilter.folder.sd.csv'
  ldpobj='$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  ldpsch='$inputDir\Ldap\$($filesPrefix)_LDAP_sch.csv'
  ldpace='$inputDir\Ldap\$($filesPrefix)_EWS_foldersd.csv'
"@
  }

    Execute-Cmd-Wrapper -cmd @"
.\Bin\Control.MakeAllNodes.exe
  -D '$logLevel'
  -L '$inputDir\Logs\$filesPrefix.makeallnodes.log'
  -I '$inputDir\Ldap\$($filesPrefix)_LDAP_obj.csv'
  -A '$((Get-ChildItem -Path $inputDir\Relations\*.csv -Exclude *.deny.csv) -join ',')'
  -O '$inputDir\Ldap\all_nodes.csv'
"@

    $globalTimer.Stop()
    Write-Output-And-Global-Log "[+] Done. Total time: $($globalTimer.Elapsed)`n"

    $obj = New-Object -TypeName PSObject
    $obj | Add-Member -MemberType NoteProperty -name 'inputDir' -value $inputDir -PassThru |
           Add-member -MemberType NoteProperty -name 'domainDnsName' -value $domainDnsName -PassThru |
           Add-Member -MemberType NoteProperty -name 'logLevel' -value $logLevel -PassThru
}