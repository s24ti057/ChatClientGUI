# Chat System

## 概要
C++とWinsockを使用したマルチクライアント対応チャットシステムです。
コンソール版クライアントとDear ImGuiによるGUI版クライアントを実装しています。
開発はコンソール版クライアントから開始し、その後GUI版クライアントへ拡張しました。

## 使用技術
- C++
- Winsock2
- Dear ImGui
- std::thread
- select
-Visual Studio Community 2026

## ファイル構成
- server.cpp : チャットサーバー本体
<主な役割>
・クライアント接続待機
・複数クライアント管理
・メッセージのブロードキャスト
・ユーザー名変更処理
・接続・切断管理

- client.cpp : コンソール版クライアント
<主な役割>
・サーバーへの接続
・メッセージ送信・受信
・スレッドによる非同期通信

通信機能の動作確認およびGUI版開発のベースとして使用しました。

- main.cpp : Dear ImGuiを使用したGUI版クライアント
<主な役割>
・GUI表示
・サーバー接続
・名前変更
・メッセージ送信(Enterキー送信)
・自動スクロール
・Disconnect機能

- README.md : このファイル

## 実装機能
- サーバー/クライアント通信
- マルチクライアント対応
- 名前変更機能
- GUIチャット
- Enterキー送信
- 自動スクロール
- Disconnect機能

## 起動方法

### サーバー

ChatServer.exe

引数:
50000

### GUIクライアント

Host:
127.0.0.1

Port:
50000

Connectボタンを押して接続

## 今後の改良予定
- サーバーGUI化
- チャットログ保存
- プライベートメッセージ機能

## 学習した内容
- TCP/IP通信の基礎
- Winsockによるソケットプログラミング
- マルチスレッドプログラミング
- selectによるイベント監視
- Dear ImGuiを用いたGUI開発
- クライアント/サーバー型アプリケーション設計

## 開発環境
- OS: Windows 11
- IDE: Microsoft Visual Studio Community 2026
- 言語: C++
- GUIライブラリ: Dear ImGui
- 通信ライブラリ: Winsock2
