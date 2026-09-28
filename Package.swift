// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "154.8037.0"
let webrtcChecksum = "5a1ff4a0e2d7023edae3b24427eb0da67d71991e2edfa8ccac26d6027b27775d"

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
