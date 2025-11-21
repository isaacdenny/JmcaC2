$cert = New-SelfSignedCertificate -DnsName "jmca.local" -CertStoreLocation "Cert:\LocalMachine\My"
$thumb = (Get-ChildItem Cert:\LocalMachine\My | Where-Object Subject -like "*jmca*").Thumbprint.Replace(" ","")
$appid = "{"+([guid]::NewGuid().ToString())+"}"
$thumb = (Get-ChildItem Cert:\LocalMachine\My | Where-Object Subject -like "*jmca*").Thumbprint.Replace(" ","")
netsh http add sslcert ipport=0.0.0.0:27015 certhash=$thumb appid=$appid

netsh http delete sslcert ipport=0.0.0.0:27015

netsh http add sslcert ipport=0.0.0.0:27015 certhash=7B7C5D3F2568D896203F50E515D512A040D75A2F certstorename=MY appid="{7bee1baa-3c7c-4f02-8433-77b6351b710b}"

netsh http add urlacl url=https://localhost:27015/ user=$env:USERNAME


PowerSHell ver 5.0 

 add-type @"
> using System.Net;
> using System.Security.Cryptography.X509Certificates;
> public class TrustAllCerts : ICertificatePolicy {
>     public bool CheckValidationResult(
>         ServicePoint srvPoint, X509Certificate certificate,
>         WebRequest request, int certificateProblem) {
>         return true;
>     }
> }

[System.Net.ServicePointManager]::CertificatePolicy = New-Object TrustAllCerts
Invoke-WebRequest https://localhost:27015