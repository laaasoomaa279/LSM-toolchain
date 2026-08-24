1- nasm -f bin boot.s -o boot.bin \
2- ./lsm build --baremetal kernel.lsm -o kernel.bin \
3- cmd /c "copy /b boot.bin + kernel.bin os.img" \
4-  $fs = [System.IO.File]::OpenWrite("os.img")
    $fs.SetLength(1474560)
    $fs.Close() \
5- & "(Your Path to Qemu Folder)\qemu\qemu-system-x86_64.exe" -fda os.img 

make sure that lsm.exe and the files are in the same folder 
