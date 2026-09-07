import bookforge_py as bf


def main() -> None:
    print("Imported bookforge_py successfully.")
    print("Exports:", bf.__all__)
    print("Training dataset builder:", bf.build_training_dataset)
    print("Chronological split:", bf.chronological_split)
    print("Kyle's Lambda estimator:", bf.estimate_kyle_lambda)


if __name__ == "__main__":
    main()
