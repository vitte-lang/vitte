# stdlib/test

Testing framework and harness utilities for Vitte.

Goals:
- deterministic unit tests
- golden tests (fixtures + normalized outputs)
- snapshot testing (text)
- microbench harness hooks
- JUnit/JSON reporting

Components:
- harness/: test discovery + runner protocol
- assert/: assertions + matchers
- golden/: golden fixture runner
- snapshot/: snapshot serializer (stub)
- report/: output formats (json/junit/text)
- util/: temp dirs, diff, normalize helpers
- samples/: example tests for integration
