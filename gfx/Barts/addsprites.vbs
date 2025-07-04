Option Explicit

Dim objFSO, objFolder, objFile, objShell
Dim strFolderPath, strT3sFilePath, strHeader
Dim colFiles, objFileItem

' Define the folder path
strFolderPath = ".\" ' Current directory

' Define the .t3s file path
strT3sFilePath = strFolderPath & "output.t3s"

' Define the header
strHeader = "--atlas -f rgba8888 -z auto" & vbCrLf

' Create FileSystemObject
Set objFSO = CreateObject("Scripting.FileSystemObject")

' Create the .t3s file
Set objFile = objFSO.CreateTextFile(strT3sFilePath, True)
objFile.Write strHeader

' Get the folder object
Set objFolder = objFSO.GetFolder(strFolderPath)
Set colFiles = objFolder.Files

' Iterate through each file in the folder
For Each objFileItem In colFiles
    If LCase(objFSO.GetExtensionName(objFileItem.Name)) = "png" Then
        objFile.WriteLine objFileItem.Name
    End If
Next

' Clean up
objFile.Close
Set objFile = Nothing
Set objFolder = Nothing
Set objFSO = Nothing

WScript.Echo "Done! .t3s file created with PNG files."
