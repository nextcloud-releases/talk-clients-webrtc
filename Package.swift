// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "135.7049.0"
let webrtcChecksum = "8f229be10a5c47dcd5be63180f788ca11b0d312e03f4205cd44b39bad696c605"

let package = Package(
    name: "WebRTC",
    platforms: [
        .iOS(.v15),
    ],
    products: [
        .library(
            name: "WebRTC",
            targets: ["WebRTC"])
    ],
    targets: [
        .binaryTarget(
            name: "WebRTC",
            url: "https://github.com/nextcloud-releases/talk-clients-webrtc/releases/download/\(webrtcVersion)/WebRTC.xcframework.zip",
            // Generate checksum with `swift package compute-checksum WebRTC.xcframework.zip`
            checksum: webrtcChecksum
        ),
    ]
)
