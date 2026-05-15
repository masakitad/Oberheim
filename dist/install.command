#!/bin/bash
#
# OB-8 Native -- macOS インストーラー
#
# 使い方:
#   1. このファイルをダブルクリック (Terminal で実行されます)
#      もしくは Terminal から:  bash install.command
#   2. 同じフォルダにある "OB-8 Native.app" の隔離属性を外し、
#      アドホック署名を打ち直して Gatekeeper を通れるようにします。
#   3. 完了後に Applications フォルダにコピーするか聞かれます。
#
# 何をしているか (技術的な話):
#   * xattr -cr      : ダウンロード/AirDrop で付いた com.apple.quarantine
#                      などの拡張属性を全て削除します。
#   * codesign       : Apple Developer ID 無しで通れるアドホック署名 (--sign -)
#                      を当てます。これでこの Mac でだけ実行が許可されます。
#
# このスクリプトはどのフォルダに置いてダブルクリックしても動きます。
# OB-8 Native.app と同じディレクトリに置いておいてください。

set -euo pipefail

# 色付きメッセージ用
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
RESET='\033[0m'

info()    { printf "${GREEN}[INFO]${RESET} %s\n" "$*"; }
warn()    { printf "${YELLOW}[WARN]${RESET} %s\n" "$*"; }
error()   { printf "${RED}[ERROR]${RESET} %s\n" "$*"; }
section() { printf "\n${BOLD}== %s ==${RESET}\n" "$*"; }

# このスクリプトが置かれているディレクトリ (.command でダブルクリックされたとき
# でも正しく解決できるよう、$0 を絶対パス化)
SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd -P )"
APP_NAME="OB-8 Native.app"
APP_PATH="${SCRIPT_DIR}/${APP_NAME}"

cd "$SCRIPT_DIR"

# ---------------------------------------------------------------------------
section "OB-8 Native インストーラー"
echo "場所: $SCRIPT_DIR"

if [[ ! -d "$APP_PATH" ]]; then
    error "\"$APP_NAME\" がこのフォルダに見つかりません。"
    error "このスクリプトを \"$APP_NAME\" と同じフォルダに置いてから"
    error "もう一度ダブルクリックしてください。"
    echo
    read -n 1 -s -r -p "Enter で終了..."
    exit 1
fi

if [[ ! -x "${APP_PATH}/Contents/MacOS" ]]; then
    error "\"${APP_NAME}\" の中身が壊れているようです (Contents/MacOS が見つかりません)。"
    echo
    read -n 1 -s -r -p "Enter で終了..."
    exit 1
fi

# ---------------------------------------------------------------------------
section "1/3  隔離属性を解除"
if xattr -cr "$APP_PATH"; then
    info "OK"
else
    error "xattr -cr に失敗しました。"
    exit 1
fi

# ---------------------------------------------------------------------------
section "2/3  アドホック署名を打ち直し"
if codesign --force --deep --sign - --timestamp=none "$APP_PATH"; then
    info "OK"
else
    error "codesign に失敗しました。"
    error "macOS の codesign コマンドがインストールされているか確認してください"
    error "(Xcode コマンドラインツールが必要)。"
    error "  xcode-select --install"
    exit 1
fi

# 署名できているか確認
if codesign --verify --verbose=2 "$APP_PATH" >/dev/null 2>&1; then
    info "署名検証 OK"
else
    warn "署名検証に失敗しました (続行はできます)"
fi

# ---------------------------------------------------------------------------
section "3/3  起動 / インストール"
echo
echo "次のいずれかを選んでください:"
echo "  [1] このフォルダで直接起動 (推奨: 試すだけならこれ)"
echo "  [2] /Applications にコピーしてから起動"
echo "  [3] 何もせず終了 (後から自分で起動する)"
echo
read -p "番号を入力 (1/2/3, 既定 1): " choice
choice="${choice:-1}"

case "$choice" in
    1)
        info "起動します..."
        open "$APP_PATH"
        ;;
    2)
        DEST="/Applications/${APP_NAME}"
        if [[ -e "$DEST" ]]; then
            warn "/Applications に同名アプリが既にあります。上書きしますか? [y/N]"
            read -p "> " overwrite
            if [[ "${overwrite:-N}" =~ ^[Yy]$ ]]; then
                rm -rf "$DEST"
            else
                warn "上書きしませんでした。終了します。"
                exit 0
            fi
        fi
        info "コピー中..."
        cp -R "$APP_PATH" "$DEST"
        info "署名を再適用 (コピー時に拡張属性が付くため)"
        xattr -cr "$DEST"
        codesign --force --deep --sign - --timestamp=none "$DEST"
        info "起動します..."
        open "$DEST"
        ;;
    3)
        info "終了します。ダブルクリックで起動できる状態です。"
        ;;
    *)
        warn "番号が違います。何もせず終了します。"
        ;;
esac

echo
info "完了です。"
echo
read -n 1 -s -r -p "Enter でこのウィンドウを閉じる..."
echo
