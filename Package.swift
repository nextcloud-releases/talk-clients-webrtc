// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "121.6167.0"
let webrtcChecksum = "f295b74eff1ff9eef14dd664834a6df413f79a65b91839d12e03b62b79639c52"

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
