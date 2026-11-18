# Publishing this prepared project

Repository: https://github.com/Mjsaber1381/SmartBuilding-GroupC

The repository exists under the correct account. Upload and Pages setup
have not completed. The prepared source archive contains the files intended
for GitHub, with private secrets and compiled firmware excluded.

When GitHub authentication is available in Git, run from the project root:

```text
git push -u origin main
```

Alternatively, extract the source archive, open the repository's **Upload
files** page, and drag the extracted project contents into it. Preserve
the `docs` and `firmware` directory structure. Commit to `main`.
Use the prepared archive rather than dragging the working firmware folder,
which also contains the private `secrets.h` and build outputs.

In repository **Settings → Pages**, choose **Deploy from a branch**, branch
`main`, directory `/docs`, and save. A private repository requires a plan
that supports Pages; otherwise its visibility must be made public by the
owner before enabling Pages.

The site address will be:
https://Mjsaber1381.github.io/SmartBuilding-GroupC/

Ask the instructor to allow origin `https://Mjsaber1381.github.io` in the
read API, and enter the separate dashboard access code in the browser.
Verify the deployed page after GitHub's Pages deployment finishes.
