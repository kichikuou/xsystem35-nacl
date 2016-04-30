## ビルド方法

1. [Native Client SDK](https://developer.chrome.com/native-client/sdk/download) をインストールします。
2. 環境変数 `NACL_SDK_ROOT` を設定しておきます。バージョン番号の部分はインストールされたSDKに合わせて変えてください。
    ```
    $ export NACL_SDK_ROOT=$(HOME)/nacl_sdk/pepper_49
    ```

3. [webports](https://chromium.googlesource.com/webports/) で必要なライブラリをインストールします。
    1. [How to Checkout](https://chromium.googlesource.com/webports/#How-to-Checkout) に書いてある手順に従ってチェックアウトします。
    2. xsystem35-nacl ではSDLを一部修正して使っています。`webports/src/ports/sdl/pkg_info`を以下のように書き換えます。

        ```
        diff --git a/ports/sdl/pkg_info b/ports/sdl/pkg_info
        index 21813f1..2658050 100644
        --- a/ports/sdl/pkg_info
        +++ b/ports/sdl/pkg_info
        @@ -1,7 +1,7 @@
         NAME=sdl
         VERSION=1.2.15
        -URL=https://github.com/sbc100/SDL-mirror.git@1c6f2d0
        +URL=https://github.com/kichikuou/SDL-mirror.git@c9145ce
         LICENSE=LGPL2
        -DEPENDS=(nacl-spawn regal)
        +DEPENDS=(nacl-spawn)
         SHA1=0c5f193ced810b0d7ce3ab06d808cbb5eef03a2c
         DISABLED_TOOLCHAIN=(emscripten)
        ```

    3. 必要なライブラリをインストールします。

        ```
        $ cd webports/src
        $ bin/webports --toolchain pnacl install zlib bzip2 freetype libogg libvorbis libpng sdl
        ```

4. xsystem35-nacl をチェックアウトしてビルドします。

    ```
    $ git clone https://github.com/kichikuou/xsystem35-nacl.git
    $ cd xsystem35-nacl/src
    $ make
    ```

    `src/pnacl/Release`ディレクトリに`xsystem35.pexe`と`xsystem35.nmf`ファイルが生成されるので、
[kichikuou.github.io](https://github.com/kichikuou/kichikuou.github.io)リポジトリをcloneしてxsystem35ディレクトリにコピーします。
