// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "144.7559.0"
let webrtcChecksum = "8c8787817a236ac36238351fa03a0f849bb68bd7fac401f962444a0faf1c5b58"

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
