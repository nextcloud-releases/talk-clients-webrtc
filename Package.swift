// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "135.7049.0"
let webrtcChecksum = "43d74eb9900b9227ba98befe5d4760f9ea14ed66c3d88f04d1805ce1fdfd38f2"

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
