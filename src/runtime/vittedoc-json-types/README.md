# doc JSON Types

This crate exposes the doc JSON API as a set of types with serde implementations.
These types are part of the public interface of the doc JSON output, and making them
their own crate allows them to be versioned and distributed without having to depend on
any c/doc internals. This way, consumers can rely on this crate for both documentation
of the output, and as a way to read the output easily, and its versioning is intended to
follow semver guarantees about the version of the format. JSON format X will always be
compatible with doc-json-types version N.

Currently, this crate is only used by doc itself. Upon the stabilization of
doc-json, it may be distributed separately for consumers of the API.
