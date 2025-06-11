// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "137.7151.0"
let webrtcChecksum = "a4e02a304c5bfb7dfeea092ec348b743a5ce70dc4b506b8e3c2768aee16fd2d7"

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
