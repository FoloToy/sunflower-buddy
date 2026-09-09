<p align="right"><strong>English</strong> · <a href="README.zh_CN.md">简体中文</a></p>

# Components

This baseline intentionally contains only one repository-owned custom component,
`bsp`. Add reusable hardware support to `components/bsp`; keep product and demo
behavior in `main`. ESP Component Manager dependencies are declared by
`components/bsp/idf_component.yml` and `main/idf_component.yml`, locked in
`dependencies.lock`, and materialized into ignored `managed_components/` output.
Do not edit that generated directory.

The [BSP module guide](bsp/README.md) documents public APIs, initialization
order, call contexts, and hardware constraints.
