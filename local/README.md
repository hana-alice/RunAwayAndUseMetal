# Local workspace

Put machine-local assets, captures, generated experiments, and other files that must not be committed under this directory.

Git ignores everything here except this convention file. Local render assets should use this layout:

```text
local/assets/models/<model-name>/
```

The checked-in sample must keep a repository-owned default asset so a fresh clone remains runnable.
