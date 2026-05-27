#!/bin/bash
#
# OB-8 Native -- macOS インストーラー (フル版)
#
# 同じフォルダに置かれた以下のうち存在するものを、Gatekeeper を通せる
# 状態に整えて適切な場所にインストールします:
#
#   - OB-8 Native.app           --> /Applications (任意)
#   - OB-8 Native.vst3          --> ~/Library/Audio/Plug-Ins/VST3/
#   - OB-8 Native.component     --> ~/Library/Audio/Plug-Ins/Components/  (Audio Unit)
#
# 何をしているか:
#   * xattr -cr  : ダウンロード/AirDrop で付いた quarantine 属性を除去
#   * codesign   : ad-hoc 署名 (--sign -) を打ち直してこの Mac で実行許可
#   * cp -R      : 規定のプラグインフォルダにコピー
#
# 使い方:
#   このファイルをダブルクリック (Finder が Terminal で開きます)
#   もしくは Terminal から  bash install.command

set -euo pipefail

# --- 色付き出力 -----------------------------------------------------------
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BOLD='\033[1m'
RESET='\033[0m'

info()    { printf "${GREEN}[INFO]${RESET} %s\n" "$*"; }
warn()    { printf "${YELLOW}[WARN]${RESET} %s\n" "$*"; }
error()   { printf "${RED}[ERROR]${RESET} %s\n" "$*"; }
section() { printf "\n${BOLD}== %s ==${RESET}\n" "$*"; }

# このスクリプトが置かれているディレクトリ
SCRIPT_DIR="$( cd "$( dirname "$0" )" && pwd -P )"
cd "$SCRIPT_DIR"

# --- 対象ファイル候補 -----------------------------------------------------
APP_NAME="OB-8 Native.app"
VST3_NAME="OB-8 Native.vst3"
AU_NAME="OB-8 Native.component"

APP_SRC="${SCRIPT_DIR}/${APP_NAME}"
VST3_SRC="${SCRIPT_DIR}/${VST3_NAME}"
AU_SRC="${SCRIPT_DIR}/${AU_NAME}"

VST3_DEST_DIR="${HOME}/Library/Audio/Plug-Ins/VST3"
AU_DEST_DIR="${HOME}/Library/Audio/Plug-Ins/Components"

# --- ヘッダ ---------------------------------------------------------------
section "OB-8 Native インストーラー"
echo "場所: $SCRIPT_DIR"

found_any=0
[[ -d "$APP_SRC"  ]] && info "発見: ${APP_NAME}"       && found_any=1
[[ -d "$VST3_SRC" ]] && info "発見: ${VST3_NAME}"      && found_any=1
[[ -d "$AU_SRC"   ]] && info "発見: ${AU_NAME} (AU)"   && found_any=1

if [[ "$found_any" -eq 0 ]]; then
    error "インストール対象が見つかりません。"
    error "このスクリプトを以下のいずれかと同じフォルダに置いてください:"
    error "  ${APP_NAME}, ${VST3_NAME}, ${AU_NAME}"
    echo
    read -n 1 -s -r -p "Enter で終了..."
    exit 1
fi

# --- ヘルパ: bundle 1 個を整備 -------------------------------------------
prepare_bundle() {
    local path="$1"
    local label="$2"
    printf "  %-32s: " "$label"
    if xattr -cr "$path" \
       && codesign --force --deep --sign - --timestamp=none "$path" 2>/dev/null; then
        printf "${GREEN}OK${RESET}\n"
        return 0
    else
        printf "${RED}FAIL${RESET}\n"
        return 1
    fi
}

# --- 1: xattr / codesign --------------------------------------------------
section "1/3  隔離属性の解除とアドホック署名"
[[ -d "$APP_SRC"  ]] && prepare_bundle "$APP_SRC"  "$APP_NAME"
[[ -d "$VST3_SRC" ]] && prepare_bundle "$VST3_SRC" "$VST3_NAME"
[[ -d "$AU_SRC"   ]] && prepare_bundle "$AU_SRC"   "$AU_NAME"

# --- 2: プラグイン本体のインストール -------------------------------------
section "2/3  プラグインのインストール"

install_plugin() {
    local src="$1"
    local dest_dir="$2"
    local label="$3"
    if [[ ! -d "$src" ]]; then
        return 0
    fi
    mkdir -p "$dest_dir"
    local dest="${dest_dir}/$(basename "$src")"
    if [[ -e "$dest" ]]; then
        warn "${label}: 既存を上書きします (${dest})"
        rm -rf "$dest"
    fi
    cp -R "$src" "$dest"
    # コピー時に属性が付くため再度クリーン + 署名
    xattr -cr "$dest"
    codesign --force --deep --sign - --timestamp=none "$dest" 2>/dev/null || true
    info "${label} -> ${dest_dir}/"
}

install_plugin "$VST3_SRC" "$VST3_DEST_DIR" "$VST3_NAME"
install_plugin "$AU_SRC"   "$AU_DEST_DIR"   "$AU_NAME"

# Standalone (.app) は任意で /Applications にコピー
if [[ -d "$APP_SRC" ]]; then
    echo
    echo "Standalone (${APP_NAME}) について:"
    echo "  [1] このフォルダで直接起動 (試すだけならこれ)"
    echo "  [2] /Applications にコピーしてから起動"
    echo "  [3] 何もしない"
    echo
    read -p "番号 (1/2/3, 既定 1): " choice
    choice="${choice:-1}"
    case "$choice" in
        1) info "現在地で起動: $APP_SRC"
           open "$APP_SRC"
           ;;
        2) DEST="/Applications/${APP_NAME}"
           if [[ -e "$DEST" ]]; then
               warn "/Applications に同名アプリが既にあります。上書きしますか? [y/N]"
               read -p "> " ow
               if [[ "${ow:-N}" =~ ^[Yy]$ ]]; then
                   rm -rf "$DEST"
               else
                   warn "上書きしませんでした。元の場所のままです。"
                   DEST="$APP_SRC"
               fi
           fi
           if [[ "$DEST" != "$APP_SRC" ]]; then
               cp -R "$APP_SRC" "$DEST"
               xattr -cr "$DEST"
               codesign --force --deep --sign - --timestamp=none "$DEST" 2>/dev/null || true
           fi
           info "起動: $DEST"
           open "$DEST"
           ;;
        3) info "Standalone は何もしません。" ;;
        *) warn "番号が無効でした。Standalone は何もしません。" ;;
    esac
fi

# --- 3: 確認メッセージ ---------------------------------------------------
section "3/3  完了"
echo
echo "VST3 と AU をインストールした場合、DAW を起動して"
echo "プラグインを再スキャンしてください:"
echo
echo "  - Logic Pro:    Audio Units の認証 (初回のみ)"
echo "  - Ableton Live: 設定 → プラグイン → 再スキャン"
echo "  - Cubase:       Studio → VST プラグインマネージャ → 更新"
echo "  - Reaper:       Preferences → Plug-ins → VST → Clear cache and re-scan"
echo
info "メーカー名: OB8Native"
info "プラグイン名: OB-8 Native"
echo
read -n 1 -s -r -p "Enter でこのウィンドウを閉じる..."
echo
