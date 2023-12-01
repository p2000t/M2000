"%WIX%bin\candle" -arch x86 product.wxs
"%WIX%bin\light" -ext WixUIExtension -ext WixUtilExtension -sacl -spdb -out M2000-installer.msi product.wixobj
