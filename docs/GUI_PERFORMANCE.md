# Prepared pages and rendering work

## Optional RGB565 pictures

PSRAM builds expose `tdui.SUPPORTS_RGB_PICTURES`. Call
`tdui.enable_rgb_pictures()` before creating windows, images or fonts to retain
RGB565 image, gradient and coarse underlay colors. Palette mode remains the
default, with unchanged legacy allocation budgets. RGB mode adds two bytes
per decoded image/coarse canvas/background pixel and a visible native
backdrop picture per two canvas rows. App makers should include these costs when their
RGB565 export option is selected; small targets export their palette fallback.

CrowPanel copies RGB565 pictures directly into its existing frame-boundary
safe LCD path. Native background pictures survive partial overlap; damage to
lower pictures redraws overlapping upper pictures in painter order. Opaque
text/primitive replacement retains its existing invalidation semantics.
Backgrounds render in two-cell-row strips: at font 20, an 800-pixel-wide
strip needs 64 KiB instead of a 750 KiB full-screen render buffer. Each strip
uses a content signature, so unchanged rows survive dynamic value updates.
The host simulator can capture the same RGB plane for browser composition.
Sixel terminals receive a per-picture palette with up to 256 source colors;
more complex pictures use RGB332. That serial fallback is not full RGB565.

The CrowPanel RGB backend now uses one lazily allocated screen composition
plane plus per-cell generations for both LCD framebuffers. Native overlays
replace their final pixels in that plane; they do not each retain another
full-size display copy or consume a legacy Sixel slot. Tests cover occupied
legacy slots and 300 repeated native replacements. On CrowPanel, the shared
plane resolved measured allocation failures and overlapping pages: 21 page
transitions completed with zero image/cache-write failures and over 1 MB free.
Image downsampling also uses one logical row of temporary pixels at a time,
instead of an enlarged copy of the complete image. A full-width font-20 RGB
fallback row needs 40 KiB. The display plane and its generation tables are
additional backend storage; `MAX_PIXELS` accounts for application resources.

Kiosk artwork uses RGB565 on PSRAM targets as well, so the startup splash and
an RGB app do not show different palette-quantized colors. Smaller targets
retain the existing one-byte source storage.

`tdui.graphics_info()` returns `(renders, allocation_failures,
sink_failures, psram_free, psram_largest, frame_ms)`. The first three values
are cumulative renderer counters; memory values are bytes, and the last
value is the most recently completed desktop frame's duration. Host builds
return zero for ESP-specific memory values. The sink counter records native
RGB sink rejections, which may still succeed through Sixel; interpret it
with the target/backend.

The host `tdsim_lcd` test executable runs the real CrowPanel VT decoder,
image cache and painter with simulated RTOS/frame boundaries. `lcdshot=PATH`
captures its visible P6 framebuffer without invalidating anything. The
`redraw` action explicitly rebuilds a complete frame for comparison. This
tests incremental rendering; it does not simulate PSRAM contention or DMA.

App packages allocate their bounded canvas/page groups, load
images/fonts once, and paint every retained page before `on_ready`. A matching
page rectangle shares a native canvas with multiple cached pictures. Selecting
a page is one atomic PAGE request that sets both drawing and shown pictures.
It does not open windows, read assets or run the static painter again. Dynamic
values are separate updates; a user's own page handlers may add work.

Control creation and logical page preparation remain eager, so `on_ready`
can access hidden controls. RGB raster warming is incremental: after the
first visible page, an idle tick may pre-render one hidden picture with at
most 160 KiB of native output. Warming yields to queued requests, drawing
batches and redraws, and tries each unchanged hidden picture once.

The optional lossless RGB565 run cache uses one arena, up to 128 KiB of
compressed pixels plus its bounded index. Identical artwork is deduplicated
after an exact pixel comparison. LRU eviction compacts that arena; entry
generations reject old handles. No per-image heap blocks are retained.
Source-image changes invalidate every referring picture, including copied
pages. Replaced text/geometry creates a fresh picture; the cache also checks
native dimensions and theme colors. Closing the app releases the arena/index.

`tdui.render_cache(kb=128, warm=True)` requests 0, 64 or 128 KiB (the API
accepts 0–128); zero disables reuse and warming. Configuration is queued to
the UI owner. Allocation requires at least 768 KiB free PSRAM at that point,
and failure simply leaves normal rendering active. The cache is backend
storage, separate from the application `MAX_PIXELS` allocation budget.
Smaller builds retain no cache arena/index. App makers can expose these
choices; existing RGB apps use the automatic policy without recompilation.

`tdui.render_cache_info()` reports capacity, used compressed bytes, hits,
misses, evictions, idle warm attempts, raster milliseconds, decode milliseconds,
picture frames, total picture-frame milliseconds, peak and last frame time.
Counters describe rendering, not physical touch-to-LCD latency. Native picture
frame timing includes raster/cache decode and the local display sink.

On CrowPanel, 30 canvas-button navigations per setting measured:

| Cache payload | Native drawing median | Handler entry to LCD submission median |
| --- | --- | --- |
| Off | 224 ms | 364 ms |
| 64 KiB | 195 ms | 320 ms |
| 128 KiB | 139.5 ms | 260.5 ms |

Submission uses MCU timestamps from the same monotonic clock as the script;
UART polling overhead is excluded. Requests settle for 600 ms before the
next; that verification interval is excluded too. These measurements begin
at synthetic button-handler entry, excluding touch sampling/finger hold and
the subsequent LCD refresh. A heavier page's first focus reached 723 ms at 128 KiB.
They describe one application/board, not an instant-page guarantee.

The selected 128 KiB setting completed 180 further page changes with live
bench values, zero allocation/sink failures and zero missed preparations.
Free PSRAM was 771,088 bytes, with a 622,592-byte largest block. Payload use
in the timing run was 129,760 of 131,072 bytes, plus the bounded index.
Physical testing confirmed responsive, stable navigation.

The direct-cache path expands cached runs straight into the
local display's composition plane, avoiding a transient decoded image and
its extra PSRAM copy. The port validates the complete run length before
writing; unsupported/rejected direct writes use the normal RGB path. The
cached span is borrowed synchronously from its UI-owned arena. Expansion
and local write time contribute to the cache decode-time counter.

On a local RGB display, fully covered canvas-background areas are omitted
when native pictures supply those exact pixels. Only uncovered cell spans
are rendered. The renderer checks picture capacity before selecting this
path and retains full two-row strips when sparse spans would overflow the
picture budget or save little work. Serial/Sixel output retains its full
background path. Pixel tests cover page replacement, transparent underlays,
hidden edits and button states. The timing table includes these paths and
the continuous-row LCD painter.

The current native fast paths avoid clearing underneath opaque pictures, fill
opaque font backgrounds by rows, copy unscaled opaque rows with `memcpy`, fill
constant-color images with `memset`, and reuse an already scaled row for
repeated source rows. Keyed/masked images retain the exact underlay path.
These changes preserve picture ownership and page-buffer lifetimes.

`TD_RENDER_OPTIMIZE=ON` builds the native raster/cache/picture and core
canvas/screen/compositor/cell-renderer hot paths
with `-O2` on GCC/Clang host builds and ESP-IDF. CrowPanel's display painter
uses the same setting. Other firmware components retain their configured
optimization/debug policy. Use `-DTD_RENDER_OPTIMIZE=OFF` in CMake when
debugging those graphics files.

`tdui.scanout_info()` is optional local-display telemetry, returning eight
cumulative values: VSYNCs, completed frame preparations, VSYNCs without a new
preparation, longest preparation gap in microseconds, submitted frames,
pixel-clock Hz, RGB interrupt CPU and internal bounce-buffer lines. Unsupported
ports/runtimes return zeroes. Startup alignment is excluded from the missing
preparation count. These counters expose long scanout stalls independently
of successful UART replies or correct rendered pixels; they do not identify
every short DMA underrun or replace physical display testing.

`tdui.scanout_timing()` returns the last LCD submission's monotonic timestamp
in milliseconds, followed by last, peak and cumulative LCD paint work in
microseconds. The timestamp uses the ESP timer clock on CrowPanel; scripts
can mask it to their `time.ticks_ms()` period and use `time.ticks_diff()`.
This measures submission to the RGB driver, with up to another refresh before
all pixels are scanned out. Unsupported runtimes/ports return four zeroes.

The LCD painter copies adjacent changed native tiles as continuous pixel
rows, retaining source-row boundaries when VT operations move image tiles.
It compares the requested image against the visible front buffer before
updating the hidden backbuffer. This suppresses duplicate presentations from
terminal status traffic and still presents a requested scene already held
in the backbuffer. Neither optimization writes into the submitted buffer.

CrowPanel uses `TD_CROWPANEL_RENDER_CORE=1` to keep
UI rendering, RGB initialization/interrupts and LCD painting on one CPU.
Setting it to `-1` restores unpinned tasks for comparison. This addresses
simultaneous bulk PSRAM copies from different CPUs. In the instrumented
workload, unpinned rendering missed nine frame preparations and reached a
140.650 ms gap. The same 180 page changes with affinity missed none, with
a 21.978 ms peak gap; physical testing confirmed no bounce. These counters
measure completed preparations, rather than every individual DMA refill.
The LCD clock remains 24 MHz.

CrowPanel already presents through RGB DMA with two LCD framebuffers. Keep its
tested 24 MHz pixel clock and frame-boundary handoff; increasing the pixel
clock previously caused a shifted logo fragment under load. ESP32-S3 has no
PPA pixel-processing accelerator. ESP32-P4 has PPA, but no P4 board is part of
this test set and no P4 accelerated renderer is claimed here.

Official references:

- [Espressif S3/P4 comparison](https://developer.espressif.com/workshops/edge-ai-with-esp32-s3/introduction/)
- [ESP32-P4 PPA driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/ppa.html)

Native tests cover opaque/masked images, clipping, rounded ends, retained
pictures and ownership, font backgrounds, batch presentation and page
switches. Hardware performance and visual smoothness also need the target
display and application; the browser preview does not model UART latency or
machine timing.

## Live native task information on CrowPanel

CrowPanel's Task Manager uses a bounded task snapshot without scanning unused
task stacks. ESP-IDF's normal `uxTaskGetSystemState` scans every stack while
holding the kernel lock; that interrupt delay can starve the RGB bounce-buffer
interrupt and briefly shift the display. The CrowPanel snapshot copies task
identity, name, state, priority, core and CPU counters under the lock, then
formats the table after releasing it. Its stack-watermark column shows `-`.
Other boards retain their stack-watermark readings.

The project supplies this helper through ESP-IDF's FreeRTOS task-additions
include hook, without modifying the installed SDK. It currently targets the
legacy ESP-IDF 5.3.1 kernel; selecting the experimental SMP kernel fails the
CrowPanel configuration explicitly rather than using incompatible internals.
Keep the 24 MHz LCD setting and existing framebuffer handoff unchanged.

Native regression tests exercise the snapshot's locking, copied names and
disabled stack scan, and built-in app layouts at 80x24 and 100x30 cells. They
also cover live Task Manager selection across window deletion/reuse, log
buffer rollover and Start-menu scrolling. A physical LCD check remains
necessary to verify scanout stability under the target workload.

- [Espressif LCD FAQ: RGB screen drift and interrupt delays](https://docs.espressif.com/projects/esp-faq/en/latest/software-framework/peripherals/lcd.html)
