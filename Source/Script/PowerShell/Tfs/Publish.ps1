# C:\Program Files (x86)\Git\bin\curl.exe
Param
(
    $FtpUser,
    [SecureString]
    $FtpPassword,
    $FtpHost
)

foreach($file in $(Get-ChildItem ..\..\artifacts\drop -Recurse -Name -Attributes !D))
{
    $uri = "${file}".Replace('\','/');
    curl.exe --silent --show-error --ftp-ssl --insecure --ftp-create-dirs -T ..\..\artifacts\drop\$uri ftp://${FtpUser}:${FtpPassword}@${FtpHost}/ci/$(Build.BuildNumber)/$uri
}