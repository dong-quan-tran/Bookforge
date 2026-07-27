import bookforge_py as bf


def main() -> None:
    print("Imported bookforge_py successfully.")
    print("Exports:", bf.__all__)
    print("DepthLevelSnapshot:", bf.DepthLevelSnapshot)
    print("BookSnapshot:", bf.BookSnapshot)
    print("FeatureRow:", bf.FeatureRow)


if __name__ == "__main__":
    main()
