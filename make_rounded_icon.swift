import AppKit

guard CommandLine.arguments.count > 2 else {
    print("Usage: make_rounded_icon <input.png> <output_dir>")
    exit(1)
}

let inputPath = CommandLine.arguments[1]
let outputDir = CommandLine.arguments[2]

guard let image = NSImage(contentsOfFile: inputPath) else {
    print("Error: Could not load image at \(inputPath)")
    exit(1)
}

let sizes = [16, 32, 64, 128, 256, 512, 1024]
let fileManager = FileManager.default

let iconsetPath = (outputDir as NSString).appendingPathComponent("icon.iconset")
try? fileManager.createDirectory(atPath: iconsetPath, withIntermediateDirectories: true)

for size in sizes {
    for scale in [1, 2] {
        let pixelSize = size * scale
        if size == 1024 && scale == 2 { continue }
        
        guard let rep = NSBitmapImageRep(
            bitmapDataPlanes: nil,
            pixelsWide: pixelSize,
            pixelsHigh: pixelSize,
            bitsPerSample: 8,
            samplesPerPixel: 4,
            hasAlpha: true,
            isPlanar: false,
            colorSpaceName: .calibratedRGB,
            bytesPerRow: 0,
            bitsPerPixel: 0
        ) else { continue }
        
        NSGraphicsContext.saveGraphicsState()
        let ctx = NSGraphicsContext(bitmapImageRep: rep)!
        NSGraphicsContext.current = ctx
        
        let rect = NSRect(x: 0, y: 0, width: pixelSize, height: pixelSize)
        let cornerRadius = CGFloat(pixelSize) * 0.22
        
        let clipPath = NSBezierPath(roundedRect: rect, xRadius: cornerRadius, yRadius: cornerRadius)
        clipPath.addClip()
        
        image.draw(in: rect, from: .zero, operation: .copy, fraction: 1.0)
        
        NSGraphicsContext.restoreGraphicsState()
        
        if let pngData = rep.representation(using: .png, properties: [:]) {
            let filename = scale == 1 ? "icon_\(size)x\(size).png" : "icon_\(size)x\(size)@2x.png"
            let filePath = (iconsetPath as NSString).appendingPathComponent(filename)
            try? pngData.write(to: URL(fileURLWithPath: filePath))
        }
    }
}
print("Successfully generated iconset at \(iconsetPath)")
