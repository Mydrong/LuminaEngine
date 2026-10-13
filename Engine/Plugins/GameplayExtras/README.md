# GameplayExtras

Optional gameplay-flavored components that aren't core engine functionality.
Anything that lives here can be disabled per-project via the `.lproject`
`Plugins` array, Runtime itself never depends on it.

## Currently bundled

- `SHealthComponent` (`Components/HealthComponent.h`), health + max health
  + optional regen. Reflected script API: `ApplyDamage(float)`, `GiveHealth(float)`.

## How to disable for a project

In your `.lproject`:

```json
"Plugins": [
    { "Name": "GameplayExtras", "Enabled": false }
]
```

Or use the **Tools → Plugin Browser** editor tool.

## Pattern for plugin API headers

`GameplayExtrasRuntimeAPI.h` follows the three-mode pattern every plugin
should adopt so it works in both modular (Debug/Development) and monolithic
(Shipping) builds:

```cpp
#pragma once
#ifdef LUMINA_MONOLITHIC
    #define MYMODULE_API
#elif defined(MYMODULE_EXPORTS)
    #define MYMODULE_API DLL_EXPORT
#else
    #define MYMODULE_API DLL_IMPORT
#endif
```

`LUMINA_MONOLITHIC` is defined by LuminaBuildTool for monolithic targets;
in that config every module is statically linked into the final exe and
dllexport/dllimport would just produce LNK4286 warnings.
