"%WIX%bin\candle" -arch x64 product64.wxs
"%WIX%bin\light" -ext WixUIExtension -ext WixUtilExtension -sacl -spdb -out M2000-installer.msi product64.wixobj
