$DEPLOYDIR = "DEPLOY"

cd C:\CraftRoot

Remove-Item -Recurse $DEPLOYDIR
windeployqt.exe -xml -texttospeech --release bin\labplot2.exe --dir $DEPLOYDIR
