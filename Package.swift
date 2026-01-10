// swift-tools-version:5.7.1
import PackageDescription

let webrtcVersion = "143.7499.0"
let webrtcChecksum = "1fc380c4b69f72aa98160aac41621342a9b25ae7eae7c1a995752a1a2de8b392"

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
