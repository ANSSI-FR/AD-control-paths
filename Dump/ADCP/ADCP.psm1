Get-ChildItem -Path $PSScriptRoot\*.ps1 | ForEach-Object {
    Write-Verbose "Importing $($_.Name)..."
    . ($_.Fullname)
}
