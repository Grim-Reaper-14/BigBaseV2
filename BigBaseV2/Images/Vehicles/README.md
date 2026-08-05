# Vehicle preview images

The D3D12 Vehicle page creates and reads this folder beside the built BigBaseV2 DLL:

```text
Images/Vehicles/
```

Add one image per vehicle using either the lowercase model name or its JOAAT hash.

Examples:

```text
adder.png
3078201489.png
B779A091.png
0xB779A091.png
```

Supported formats are PNG, JPG/JPEG, BMP, GIF, TIFF, and ICO. Images are decoded with Windows Imaging Component, uploaded to the shared D3D12 SRV heap only when selected, and retained in a small LRU cache. Use **Refresh Preview** on the Vehicle page after adding or replacing files while the menu is running.
