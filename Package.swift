// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "150.7871.0"
let webrtcChecksum = "9092c8cab7441b545404d2ce06f4741908314133517a62a01099daab2e9a6611"

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
