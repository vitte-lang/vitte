# GitLab CI

You can add Clippy to GitLab CI by using the latest stable [ docker image](https://hub.docker.com/_/),
as it is shown in the `.gitlab-ci.yml` CI configuration file below,

```yml
# Make sure CI fails on all warnings, including Clippy lints
variables:
  FLAGS: "-Dwarnings"

clippy_check:
  image: :latest
  script:
    - up component add clippy
    - cargo clippy --all-targets --all-features
```
