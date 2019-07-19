# https://cdn.azul.com/zulu/bin/zulu8.36.0.1-ca-jdk8.0.202-win_x64.zip
$jdkFile = "zulu8.36.0.1-ca-jdk8.0.202-win_x64.zip"
$jdkFolder = "zulu8.36.0.1-ca-jdk8.0.202-win_x64"
# https://neo4j.com/artifact.php?name=neo4j-community-3.5.3-windows.zip
$neo4jFile = "neo4j-community-3.5.3-windows.zip"
$neo4jFolder = "neo4j-community-3.5.3"
$sourcePath = $PSScriptRoot

function Install-ADCPJava {
    Param(
        [Parameter(Mandatory=$False, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$destPath = (Join-Path -Path $env:TEMP -ChildPath 'ADCP')
    )

    # Expand JRE
    If (-not (Test-Path -Path $destPath/$jdkFolder/bin)) {
        Expand-Archive -LiteralPath $sourcePath\$jdkFile -DestinationPath $destPath
    }
}

function New-ADCPInstance {
    Param(
        [Parameter(Mandatory=$False, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$destPath = (Join-Path -Path $env:TEMP -ChildPath 'ADCP')
    )

    Install-ADCPJava -destPath $destPath

    $baseinstancePath = Join-Path $destPath "instances"

    $instanceID = 1
    while($true) {
        $instancePath = Join-Path -Path $baseinstancePath -ChildPath $instanceID
        if (-Not (Test-Path -Path $instancePath)) {
            break
        }

        $instanceID++
    }

    $null = New-item -ItemType Directory -Path $instancePath
    Expand-Archive -LiteralPath $sourcePath\$neo4jFile -DestinationPath $instancePath

    Import-Module $instancePath\$neo4jFolder\bin\Neo4j-Management -Force
    Set-Neo4jEnv -Name "JAVA_HOME" -Value $destPath\$jdkFolder
    Remove-Item Env:NEO4J_CONF -ErrorAction SilentlyContinue
    Set-Neo4jEnv -Name "NEO4J_HOME" -Value $instancePath\$neo4jFolder

    # Confirm-JavaVersion -Path (Get-Java).java

    # Set Config in instance folder
    (Get-Content -Path $PSScriptRoot\"neo4j.conf") `
        -replace '<LISTEN_ADDRESS>', 'localhost' `
        -replace '<BOLT_PORT>', (7687 + $instanceID) `
        -replace '<WEB_PORT>', (7474 + $instanceID) `
        -replace '<SHELL_PORT>', (1337 + $instanceID) `
        | Set-Content -Path $instancePath\$neo4jFolder\"conf\neo4j.conf"

    $obj = New-Object -TypeName PSObject
    $obj | Add-Member -MemberType NoteProperty -Name 'instanceID' -Value $instanceID -PassThru
}

function List-ADCPInstances {
    Param(
        [Parameter(Mandatory=$False, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$destPath = (Join-Path -Path $env:TEMP -ChildPath 'ADCP')
    )

    $instancePath = Join-Path $destPath "instances"

    Get-ChildItem -Path $instancePath -Directory | ForEach-Object {
        $meta = Get-Content -Path (Join-Path -Path $_.FullName -ChildPath "meta.json") -Raw | ConvertFrom-Json

        $obj = New-Object -TypeName PSObject
        $obj | Add-Member -MemberType NoteProperty -Name 'instanceID' -Value $_.BaseName -PassThru |
               Add-Member -MemberType NoteProperty -Name 'instancePath' -Value $_.FullName -PassThru |
               Add-Member -MemberType NoteProperty -Name 'domainDnsName' -Value $meta.domainDnsName -PassThru
    }
}

function Remove-ADCPInstance {
    Param(
        [Parameter(Mandatory=$False, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$destPath = (Join-Path -Path $env:TEMP -ChildPath 'ADCP'),
        [Parameter(Mandatory=$true, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$instanceID = $null
    )

    $instancePath = Join-Path -Path $destPath -ChildPath 'instances'
    $instancePath = Join-Path -Path $instancePath -ChildPath $instanceID
    if (-Not (Test-Path -Path $instancePath)) {
        Write-Error "ERROR: Invalid InstanceID"
        return
    }

    Remove-Item -Path $instancePath -Recurse -Force
}

function Start-ADCPInstance {
    Param(
        [Parameter(Mandatory=$true, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$instanceID = $null,
        [Parameter(Mandatory=$False, ValueFromPipeline=$True, ValueFromPipelineByPropertyName=$True)][string]$destPath = (Join-Path -Path $env:TEMP -ChildPath 'ADCP')
    )

    $instancePath = Join-Path -Path $destPath -ChildPath 'instances'
    $instancePath = Join-Path -Path $instancePath -ChildPath $instanceID

    Write-Host "$instancePath $instanceID"
    if (-Not (Test-Path -Path $instancePath)) {
        Write-Error "ERROR: Invalid InstanceID"
        return
    }

    $job_script =[scriptblock]::Create("
        `$host.UI.RawUI.WindowTitle = 'Neo4j for instance $instanceID'
        Import-Module $instancePath\$neo4jFolder\bin\Neo4j-Management -Force
        Set-Neo4jEnv -Name 'JAVA_HOME' -Value $destPath\$jdkFolder
        Remove-Item Env:NEO4J_CONF -ErrorAction SilentlyContinue
        Set-Neo4jEnv -Name 'NEO4J_HOME' -Value $instancePath\$neo4jFolder

        # Launch Neo4j
        Invoke-Neo4j console
    ")

    $proc = Start-Process powershell $job_script
    $proc
}
