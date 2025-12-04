$scriptContent = Get-Content -Path 'scripts\enumservices.ps1' -Raw
$bytes = [System.Text.Encoding]::Unicode.GetBytes($scriptContent)
$encodedScript = [Convert]::ToBase64String($bytes)
Write-Output $encodedScript