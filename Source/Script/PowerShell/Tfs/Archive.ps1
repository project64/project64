Param
(
    $FtpUser,
    [SecureString]
    $FtpPassword,
    $FtpHost
)

mkdir $(Agent.BuildDirectory)\zip;

Add-Type -Assembly 'System.IO.Compression.FileSystem';

[System.IO.Compression.ZipFile]::CreateFromDirectory("$(Agent.BuildDirectory)\artifacts\drop\", "$(Agent.BuildDirectory)\zip\$(Build.BuildNumber).zip\");


curl.exe --silent --show-error --ftp-ssl --insecure --ftp-create-dirs -T $(Agent.BuildDirectory)\zip\$(Build.BuildNumber).zip ftp://${FtpUser}:${FtpPassword}@${FtpHost}/ci/$(Build.BuildNumber).zip