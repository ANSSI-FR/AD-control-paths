
function Import-ADCPDump {
    <#
    .SYNOPSIS

        Import ADCP Results in neo4j database

    .DESCRIPTION

        .

    .PARAMETER inputDir

        Output Directory of Dump.ps1

    .EXAMPLE

        .

    #>

    Param(
        [Parameter(Mandatory=$True, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$inputDir = $null,
        [string]$destPath = (Join-Path -Path $env:TEMP -ChildPath 'ADCP'),
        [Parameter(Mandatory=$false, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$domainDnsName = $null
    )

    # Ensure Java is here
    Install-ADCPJava -destpath $destPath
    $instanceID = (New-ADCPInstance -destpath $destPath).instanceID

    # Expand Neo4j for this instance
    $instancePath = Join-Path $destPath "instances"
    $instancePath = Join-Path $instancePath $instanceID

    # Import in Neo4j
    If (-not (Test-Path -Path $instancePath\$neo4jFolder\"data\databases\graph.db")) {
        Invoke-Neo4jAdmin import `
            --mode=csv `
            --id-type string `
            --nodes "$inputDir/Ldap/all_nodes.csv" `
            --relationships $((Get-ChildItem -Path $inputDir\Relations\*.csv -Exclude *.deny.csv) -join ',') `
            --input-encoding UTF-16LE `
            --multiline-fields=true `
            --ignore-missing-nodes=true `
            --report-file "$inputDir/Logs/neo4j-import-report.log"

            $meta = @{
                domainDnsName = $domainDnsName
            }
            $meta | ConvertTo-Json | Set-Content -Path (Join-Path -Path $instancePath -ChildPath "meta.json")
    }

    $obj = New-Object -TypeName PSObject
    $obj | Add-Member -MemberType NoteProperty -name 'inputDir' -value $outputDir -PassThru |
           Add-member -MemberType NoteProperty -name 'domainDnsName' -value $domainDnsName -PassThru |
           Add-Member -MemberType NoteProperty -name 'instanceID' -value ([string]$instanceID) -PassThru |
           Add-Member -MemberType NoteProperty -name 'destPath' -value $destPath -PassThru
}
