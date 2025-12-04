Add-Type -AssemblyName System.Windows.Forms,System.Drawing

$screens = [Windows.Forms.Screen]::AllScreens

$top    = ($screens.Bounds.Top    | Measure-Object -Minimum).Minimum
$left   = ($screens.Bounds.Left   | Measure-Object -Minimum).Minimum
$right  = ($screens.Bounds.Right  | Measure-Object -Maximum).Maximum
$bottom = ($screens.Bounds.Bottom | Measure-Object -Maximum).Maximum

$bounds   = [Drawing.Rectangle]::FromLTRB($left, $top, $right, $bottom)
$bmp      = New-Object System.Drawing.Bitmap ([int]$bounds.width), ([int]$bounds.height)
$graphics = [Drawing.Graphics]::FromImage($bmp)

$graphics.CopyFromScreen($bounds.Location, [Drawing.Point]::Empty, $bounds.size)

$bmp.Save("$env:TEMP\test.png")

$graphics.Dispose()
$bmp.Dispose()
