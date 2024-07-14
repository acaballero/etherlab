::Deprecated. Use swoparser.py as a compound build target instead

:: Deletes the las SWO log file
del c:/temp/swo.log

:: Creates a new one
echo .>c:/temp/swo.log


:: tails the SWO log output
powershell Get-Content c:/temp/swo.log -Wait -Tail 1