# Configuration

The configuration is a JSON file with a dictionnary of value. Here is the
default configuration:

```json
{
  "redis": {
    "enabled": true
  },
  "scheduler_submission": {
    "enabled": false,
    "acknowledge": true
  }
}
```

This configuration can be override using the ``--config_file`` option. Each
field present in the file will be used and the fields that are not provided
keeps the default value.
