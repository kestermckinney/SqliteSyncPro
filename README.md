# SqliteSyncPro

[![Project site](https://img.shields.io/badge/Project%20Site-Live-2ecc71?style=for-the-badge)](https://kestermckinney.github.io/SqliteSyncPro/)
[![Latest release](https://img.shields.io/github/v/release/kestermckinney/SqliteSyncPro?style=for-the-badge)](https://github.com/kestermckinney/SqliteSyncPro/releases)
[![Stars](https://img.shields.io/github/stars/kestermckinney/SqliteSyncPro?style=for-the-badge)](https://github.com/kestermckinney/SqliteSyncPro/stargazers)
[![Forks](https://img.shields.io/github/forks/kestermckinney/SqliteSyncPro?style=for-the-badge)](https://github.com/kestermckinney/SqliteSyncPro/network/members)
[![Open issues](https://img.shields.io/github/issues/kestermckinney/SqliteSyncPro?style=for-the-badge)](https://github.com/kestermckinney/SqliteSyncPro/issues)
[![Last commit](https://img.shields.io/github/last-commit/kestermckinney/SqliteSyncPro?style=for-the-badge)](https://github.com/kestermckinney/SqliteSyncPro/commits/main)

SqliteSyncPro is a Qt/C++ synchronization library for applications that need local SQLite responsiveness and remote PostgreSQL alignment. It is built for offline-first desktop and mobile scenarios where synchronization, authentication, and practical deployment details matter.

## Explore

- [Project site](https://kestermckinney.github.io/SqliteSyncPro/)
- [Report a bug](https://github.com/kestermckinney/SqliteSyncPro/issues/new?template=bug_report.yml)
- [Request an enhancement](https://github.com/kestermckinney/SqliteSyncPro/issues/new?template=enhancement_request.yml)
- [Ask a usage question](https://github.com/kestermckinney/SqliteSyncPro/issues/new?template=question.yml)
- [Contributing guide](CONTRIBUTING.md)

## What SqliteSyncPro Does

- Synchronizes local SQLite databases with remote PostgreSQL services through a push-pull design.
- Supports PostgREST and Supabase-style deployment patterns.
- Uses a background worker architecture so applications can stay responsive during sync.
- Includes JWT authentication, optional row encryption, and schema-aware row serialization.
- Ships with tests, an admin tool, and an example integration application.

## Activity And Request Snapshot

[![Open bugs](https://img.shields.io/github/issues-search?query=repo%3Akestermckinney%2FSqliteSyncPro+is%3Aissue+is%3Aopen+%22%5BBug%5D%3A%22+in%3Atitle&label=open%20bugs&style=flat-square)](https://github.com/kestermckinney/SqliteSyncPro/issues?q=is%3Aissue%20is%3Aopen%20%22%5BBug%5D%3A%22%20in%3Atitle)
[![Open enhancements](https://img.shields.io/github/issues-search?query=repo%3Akestermckinney%2FSqliteSyncPro+is%3Aissue+is%3Aopen+%22%5BEnhancement%5D%3A%22+in%3Atitle&label=open%20enhancements&style=flat-square)](https://github.com/kestermckinney/SqliteSyncPro/issues?q=is%3Aissue%20is%3Aopen%20%22%5BEnhancement%5D%3A%22%20in%3Atitle)
[![Open questions](https://img.shields.io/github/issues-search?query=repo%3Akestermckinney%2FSqliteSyncPro+is%3Aissue+is%3Aopen+%22%5BQuestion%5D%3A%22+in%3Atitle&label=open%20questions&style=flat-square)](https://github.com/kestermckinney/SqliteSyncPro/issues?q=is%3Aissue%20is%3Aopen%20%22%5BQuestion%5D%3A%22%20in%3Atitle)

These badges update automatically from GitHub so visitors can quickly see activity, open requests, and whether the project is moving.

## Contributing

Contributions are welcome in several forms:

- Bug reports with reproducible sync scenarios
- Enhancement requests grounded in real application architecture needs
- Test improvements and documentation additions
- Focused pull requests for sync behavior, admin tooling, or examples

Please start with [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.
