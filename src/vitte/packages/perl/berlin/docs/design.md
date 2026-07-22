# Berlin Design

Berlin stores byte buffers as plain hashes containing a byte array and cursor.
This keeps values inspectable, serializable, and easy to test.

All byte inputs are masked to `0..255`. Packing helpers are explicit about
endianness to avoid hidden platform behavior.
