# Python environment

Create a new virtual environment

```bash
python3 -m venv venv
```

and install the requirments:

```bash
pip install -r movement-tester/requirements.txt
```

# CLion run configuration

A run configuraiton for CLion is included in the repository. It uses the python interpreter which is configured in the
**Settings | Build, Execution, Deployment | Python Interpreter**, make sure the selected environment has all the
requirements installed. You can also create a new virtual environment in here.