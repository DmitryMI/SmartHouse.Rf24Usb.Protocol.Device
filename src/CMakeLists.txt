set(CMAKE_CXX_STANDARD 23)

include("${_build_cmake_local}")

set(SmartHouse.Rf24Usb.Protocol.Device.Sources 
	"SmartHouse/Rf24Usb/Protocol/Device.cpp"
	)

add_library(SmartHouse.Rf24Usb.Protocol.Device ${SmartHouse.Rf24Usb.Protocol.Device.Sources} )
target_include_directories(SmartHouse.Rf24Usb.Protocol.Device PUBLIC ../public)

SmartHouseAddDependency("https://github.com/DmitryMI/SmartHouse.Rf24Usb.Protocol" "develop")
target_link_libraries(SmartHouse.Rf24Usb.Protocol.Device PUBLIC SmartHouse.Rf24Usb.Protocol)