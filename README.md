## ビルド方法

1. [Native Client SDK](https://developer.chrome.com/native-client/sdk/download) をインストールします。
2. 環境変数 `NACL_SDK_ROOT` を設定しておきます。バージョン番号の部分はインストールされたSDKに合わせて変えてください。
    ```
    $ export NACL_SDK_ROOT=$(HOME)/nacl_sdk/pepper_43
    ```

3. [naclports](https://code.google.com/p/naclports/) で必要なライブラリをインストールします。
    1. [Checkout Process](https://code.google.com/p/naclports/wiki/HowTo_Checkout?tm=4) に書いてある手順に従ってチェックアウトします。
    2. xsystem35-nacl ではSDLを一部修正して使っています。`naclports/src/ports/sdl/pkg_info`を以下のように書き換えます。

        ```
        diff --git a/ports/sdl/pkg_info b/ports/sdl/pkg_info
        index a055ead..70e74e9 100644
        --- a/ports/sdl/pkg_info
        +++ b/ports/sdl/pkg_info
        @@ -1,6 +1,6 @@
         NAME=sdl
         VERSION=1.2.15
        -URL=https://github.com/sbc100/SDL-mirror.git@b3ef807
        +URL=https://github.com/kichikuou/SDL-mirror.git@ba33b3c
         LICENSE=LGPL2
        -DEPENDS=(nacl-spawn regal)
        +DEPENDS=(nacl-spawn)
         SHA1=0c5f193ced810b0d7ce3ab06d808cbb5eef03a2c
        ```

    3. 必要なライブラリをインストールします。

        ```
        $ cd naclports/src
        $ bin/naclports --toolchain pnacl install zlib bzip2 freetype libogg libvorbis libpng sdl
        ```

4. xsystem35-nacl をチェックアウトしてビルドします。

    ```
    $ git clone https://github.com/kichikuou/xsystem35-nacl.git
    $ cd xsystem35-nacl/src
    $ make
    ```

    `src/pnacl/Release`ディレクトリに`xsystem35.pexe`と`xsystem35.nmf`ファイルが生成されるので、
[kichikuou.github.io](https://github.com/kichikuou/kichikuou.github.io)リポジトリをcloneしてxsystem35ディレクトリにコピーします。
