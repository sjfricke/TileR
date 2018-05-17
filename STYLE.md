# Style Guide Rules

### Classes
- **UpperCapitalCamel**
- `class NewClass { }`

### Functions
- Public/Protected
	- **UpperCapitalCamel**
	- `void GetInfo();`
- Private
	- **lowerCamelCase**
	- `void internalHelper();`

### Variables

All **lowerCamelCase**

- Constants
	- **ALL_CAPS**
- Pointers
	- start with **p**
	- `pValue`
- Member of classes
	- start with **m**
	- `mValue`
	- if pointer then `mpValue`
- Globals
	- start with **g**
	- `gGlobalValue`
- Static
	- start with **s**
	- `sValue`

### Brackets

> Using Linux kernel methods

- Function new line
  - ```
	void myFunction()
	{
		// ...
	}
	```
- If and Switch statements same line
  - ```
	if (a > 0) {
		// ...
	} else if (b > 0) {
		// ...
	} else {
		// ...
	}
	```
