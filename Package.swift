// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "135.7049.0"
let webrtcChecksum = "22e4a0fb0514402f47a8f739dbf3011f4c5617d7c5bcfffed406cf5c11ba7c54"

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
