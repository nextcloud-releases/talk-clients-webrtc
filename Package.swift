// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "155.8059.0"
let webrtcChecksum = "80bc9ae1cdd370581474eebaeae4454447f89f4426e40d14ed7204ec2d33f1a1"

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
