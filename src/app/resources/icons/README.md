# Resources

## Dash Type Icons (`type`)

Upstream repository: <https://github.com/Kapeli/Dash-X-Platform-Resources>

Optimized with:

```shell
find . -type f -iname '*.png' -exec pngcrush -ow -rem allb -reduce {} \;
find . -type f -name "*.png" -exec convert {} -strip {} \;
optipng *.png
```

The following icons are renamed:

* `Enum` to `Enumeration`
* `Struct` to `Structure`
