"""Publish CI-built assets as a release; keep it draft until all uploads finish."""
import json
import os
from pathlib import Path
from urllib.parse import quote
from urllib.request import Request, urlopen


def main():
    token = os.environ['GITHUB_TOKEN']
    repo = os.environ['GITHUB_REPOSITORY']
    tag = os.environ['RELEASE_TAG']
    api = f'https://api.github.com/repos/{repo}/releases'

    def request(url, data, method='POST', content_type='application/json'):
        req = Request(url, data=data, method=method, headers={
            'Authorization': f'Bearer {token}',
            'Accept': 'application/vnd.github+json',
            'Content-Type': content_type,
            'X-GitHub-Api-Version': '2022-11-28',
        })
        with urlopen(req, timeout=120) as response:
            return json.load(response)

    assets = sorted(Path('build/release').iterdir())
    if not assets or any(p.stat().st_size == 0 for p in assets):
        raise RuntimeError('Release assets missing or empty')
    release = request(api, json.dumps({
        'tag_name': tag, 'name': f'{tag} — ESP32-C3 Lidar Bridge, 500 м',
        'body': Path('RELEASE_NOTES.md').read_text(), 'draft': True,
    }).encode())
    upload = release['upload_url'].split('{', 1)[0]
    for asset in assets:
        request(upload + '?name=' + quote(asset.name), asset.read_bytes(),
                content_type='application/octet-stream')
    published = request(api + '/' + str(release['id']), b'{"draft":false}', method='PATCH')
    print(published['html_url'])


if __name__ == '__main__':
    main()
