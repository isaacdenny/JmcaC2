$cert = New-SelfSignedCertificate -DnsName "jmca.local" -CertStoreLocation "Cert:\LocalMachine\My"
$thumb = (Get-ChildItem Cert:\LocalMachine\My | Where-Object Subject -like "*jmca*").Thumbprint.Replace(" ","")
$appid = "{"+([guid]::NewGuid().ToString())+"}"
$thumb = (Get-ChildItem Cert:\LocalMachine\My | Where-Object Subject -like "*jmca*").Thumbprint.Replace(" ","")
netsh http add sslcert ipport=0.0.0.0:27015 certhash=$thumb appid=$appid