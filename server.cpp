// server.cpp
// マルチクライアントに対応したチャットサーバ
// select関数を用いて非同期通信を行い、スレッドは使用しない

//
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib") // Windowsのソケット通信ライブラリを使う宣言

#define BUFSIZE 256
#define MAX_CLIENTS 10

SOCKET client_socks[MAX_CLIENTS]; // 複数クライアントのソケットを保持する配列
char client_names[MAX_CLIENTS][32]; //クライアント名を変更する配列
int client_count = 0;


int main(int argc, char* argv[]) 
{
    // Windowsのネットワーク機能を初期化
    WSADATA wsaData; // Windowsのネットワーク機能を使うための準備用変数

    // Winsock 2.2を使うために初期化する
    // 戻り値が0なら成功、0以外なら失敗
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 初期化に失敗した場合
        std::cout << "WSAStartup failed\n";
        return 1;
    }

    SOCKET listening_socket; // socket() が返すファイル識別子
    uint16_t port; // ポート番号
    int temp = 1;

    // 引数チェック(ポート番号が指定されているか)
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        std::exit(EXIT_FAILURE);
    }

    port = static_cast<uint16_t>(std::stoi(argv[1])); // 文字列で受け取ったポート番号を整数に変換

    // ソケットの作成: INET ドメイン・ストリーム型
    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket == INVALID_SOCKET) {
        std::cerr << "server: socket failed\n";
        std::exit(EXIT_FAILURE);
    }

    // ソケットオプションの設定
    // SO_REUSEADDR を指定しておかないと、サーバの異常終了後に再起動した場合
    // 数分間ポートがロックされ bind() に失敗することがある
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&temp, sizeof(temp)) == SOCKET_ERROR) {
        std::cerr << "server: setsockopt failed\n";
        std::exit(EXIT_FAILURE);
    }

    // サーバプロセスのソケットアドレス情報の設定
    sockaddr_in server{}; // サーバーアドレス情報構造体(server{})の初期化
    server.sin_family = AF_INET; // プロトコルファミリの設定
    server.sin_port = htons(port); // ポート番号の設定
    server.sin_addr.s_addr = htonl(INADDR_ANY); // どのIPアドレスからも接続可

    // ソケットにアドレスをバインド
    if (bind(listening_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "server: bind failed\n";
        std::exit(EXIT_FAILURE);
    }

    // 接続要求の受け入れ準備
    // バインドされたソケットを待機状態に
    if (listen(listening_socket, 5) == SOCKET_ERROR) {
        std::cerr << "server: listen failed";
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Server started. Waiting for clients. . .\n";

    bool server_running = true;

    // メインループ(クライアント接続・受信をselectで監視)
    while (server_running) {
        fd_set rfds; // 監視対象のファイルディスクリプタ集合
        FD_ZERO(&rfds);  // 集合を空に初期化
        FD_SET(listening_socket, &rfds); // 新規接続を監視するため listening_socket を追加
        int maxfd = listening_socket; // select の第1引数用に最大FDを記録

        // 既存クライアントのソケットを監視対象に追加
        for (int i = 0; i < client_count; i++) {
            FD_SET(client_socks[i], &rfds); // クライアントのソケットを監視対象に追加
            if (client_socks[i] > maxfd) maxfd = client_socks[i]; // 最大FDを更新
        }

        // select() で「誰かが接続 or 誰かが送信」するまで待つ
        // select()は全FDを監視するため、最大FD番号(maxfd)+1を渡す必要がある
        // -新しいクライアントが接続してきた
        // -既存クライアントがメッセージを送ってきた
        // -クライアントが切断した
        //この3つのどれかが起きた場合のみselect()は値を返す(以下でこれらの3つをイベントと呼ぶ)
        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) == SOCKET_ERROR) {
            std::cerr << "server: select failed\n";
            std::exit(EXIT_FAILURE);
        }

        // FD_ISSET() : selectが教えてくれた"イベントが起こったソケット"を判定する関数
        // 新規クライアントから接続要求が来た場合
        // select が"listening_socket にイベントがあるよ"と教えてくれたときに、その接続を accept して、新しいクライアントソケットを配列に登録する
        if (FD_ISSET(listening_socket, &rfds)) { // 新しい接続要求(connect)が来て、listening_socket が読み込み可能になっているか
            sockaddr_in client{}; // クライアントプロセスのソケットアドレス情報構造体の初期化
            int fromlen = sizeof(client);
            
            // accept()が返すnew_sockがその"クライアント専用の通信用ソケット"になる
            SOCKET new_sock = accept(listening_socket, (sockaddr*)&client, &fromlen);

            // クライアント配列に登録する
            if (new_sock != INVALID_SOCKET && client_count < MAX_CLIENTS) {
                client_socks[client_count] = new_sock; // 新しいクライアントを配列に追加
                std::snprintf(client_names[client_count], 32, "Client%d", client_count + 1); // デフォルト名を設定
                client_count++;
                std::cout << "Client connected. Total: " << client_count << "\n"; // 何人接続しているかを可視化
            }
        }

        // 既存クライアントからのメッセージ受信処理
        for (int i = 0; i < client_count; i++) {
            if (FD_ISSET(client_socks[i], &rfds)) { // このクライアントからデータが届いたか？
                char buf[BUFSIZE];
                int n = recv(client_socks[i], buf, BUFSIZE, 0);

                if (n <= 0) { // 切断された場合
                    closesocket(client_socks[i]); // ソケットを閉じる
                    client_socks[i] = client_socks[client_count - 1]; // 配列を詰める
                    client_count--;
                    std::cout << "Client disconnected. Total: " << client_count << "\n";

                    // 全員切断した場合サーバ終了
                    if (client_count == 0) {
                        std::cout << "All client disconnected. Server shutting down.\n";
                        server_running = false;
                    }
                    break;
                }

                buf[n] = '\0'; // 文字列終端(nはrecv()で受信したバイト数)受信データを文字列として扱う

                if (strncmp(buf, "setname ", 8) == 0) { // 受信した文字列の先頭8文字が"setname"かどうかを判定
                    char* newname = buf + 8;
                    // newnameの中で最初に'\n'が出てくる位置を探し、そこに'\0'を代入
                    newname[strcspn(newname, "\n")] = '\0'; // 名前を文字列に変換

                    if (strlen(newname) > 0) {
                        strncpy(client_names[i], newname, 31); // 配列に名前をコピー
                        client_names[i][31] = '\0'; // 必ず終端'\0'を保証している

                        // クライアント名変更メッセージを1つの文字列として組み立てる
                        std::string msg =
                            std::string("Client") + std::to_string(i + 1) + " is now known as " + client_names[i] + "\n";

                        // 名前変更を他のクライアントに通知
                        for (int j = 0; j < client_count; j++) {
                            if (j != i) send(client_socks[j], msg.c_str(), msg.size(), 0);
                        }
                    }
                    continue; // setnameは通常メッセージとして扱わない
                }

                // 名前付きでブロードキャスト
                std::string msg = std::string(client_names[i]) + ": " + buf;

                for (int j = 0; j < client_count; j++) {
                    if (j != i) send(client_socks[j], msg.c_str(), msg.size(), 0);
                }
            }
        }
    }
    closesocket(listening_socket);

    WSACleanup(); // Windowsのネットワーク機能の終了処理

    return 0;
}
