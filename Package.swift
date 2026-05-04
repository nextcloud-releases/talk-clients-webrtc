// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "147.7727.0"
let webrtcChecksum = "2435fdd247ae32432ef49dd8bae8fb02c8decf8fef074063a2df6ba9448f90c5"

let package = Package(
    name: "WebRTC",
    platforms: [
        .iOS(.v16),
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
