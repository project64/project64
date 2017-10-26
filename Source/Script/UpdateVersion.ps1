Param
(
	$SolutionDir = $(Get-Location),
	[Parameter(Mandatory=$true)]
	[string] $Version,
	[string[]] $Files = (
		'Source\Project64-core\version.h',
		'Source\nragev20\version.h',
		'Source\RSP\version.h',
		'Source\Project64-video\version.h'
	)
)

$versionArr = $Version.Split('.')
foreach ($file in $Files) {
	(Get-Content "$SolutionDir\$file") `
		-replace '(#define\s+VERSION_MAJOR\s+)\d+',    "`${1}$($versionArr[0])" `
		-replace '(#define\s+VERSION_MINOR\s+)\d+',    "`${1}$($versionArr[1])" `
		-replace '(#define\s+VERSION_REVISION\s+)\d+', "`${1}$($versionArr[2])" `
		-replace '(#define\s+VERSION_BUILD\s+)\d+',    "`${1}$($versionArr[3])" `
	| Set-Content "$SolutionDir\$file"
}
