// Renders packaging/macos/dmg-background.svg into the two PNGs used for the
// window of the macOS disk image (660x400 and 1320x800 for Retina screens).
// The PNGs are committed, so this is only needed when the SVG changes.
//
// Usage (macOS 14 or newer, from the repository root):
//   swift scripts/generate-dmg-background.swift
import AppKit

let dir = URL(fileURLWithPath: "packaging/macos")
let svg = dir.appendingPathComponent("dmg-background.svg")
guard let image = NSImage(contentsOf: svg) else {
    FileHandle.standardError.write("Cannot read \(svg.path)\n".data(using: .utf8)!)
    exit(1)
}

func render(scale: Int, to name: String) {
    let width = 660 * scale, height = 400 * scale
    guard let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: width, pixelsHigh: height,
                                     bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
                                     colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0) else { exit(1) }
    rep.size = NSSize(width: 660, height: 400) // points: 72 dpi at 1x, 144 dpi at 2x
    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: rep)
    image.draw(in: NSRect(x: 0, y: 0, width: 660, height: 400))
    NSGraphicsContext.restoreGraphicsState()
    let out = dir.appendingPathComponent(name)
    guard let data = rep.representation(using: .png, properties: [:]) else { exit(1) }
    do { try data.write(to: out) } catch {
        FileHandle.standardError.write("Cannot write \(out.path): \(error)\n".data(using: .utf8)!)
        exit(1)
    }
    print("Wrote \(out.path) (\(width)x\(height))")
}

render(scale: 1, to: "dmg-background.png")
render(scale: 2, to: "dmg-background@2x.png")
