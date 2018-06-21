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
            #Break
        } Finally {
            #$timer.Stop()
            Write-Output "*"
            Write-Output-And-Global-Log "* Time   : $($timer.Elapsed)"
            if($error) {
                Write-Output-And-Global-Log "* Return : FAIL - $error"
                #$globalTimer.Stop()
                Write-Output-And-Global-Log "[+] Done. Total time: $($globalTimer.Elapsed)`n"
            } else {
                Write-Output-And-Global-Log "* Return : OK - $LASTEXITCODE"
            }
            Write-Output-And-Global-Log "********************`n"
        }
    }
}
