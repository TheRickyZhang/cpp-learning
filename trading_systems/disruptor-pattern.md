Popularized by LMAX

Similar to SPMC queue, but instead of each item being processed once, here every item is processed by every consumer (sort of like broadcast)

For high-performance lock-free concurrency:
- Have fixed-size circular buffer
- Producers and consumers have sequence numbers
- Producer will only produce and advance marker if it has not lapped slowest consumer
- Consumers process events at their own pace, up to current marker


