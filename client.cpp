// client.cpp
// マルチクライアント用に改造したクライアントプログラム
// スレッドを用いて非同期通信を実装
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")  // Winsock ライブラリをリンク

#define BUFSIZE 256

SOCKET sock; // サーバと接続したソケット

// スレッドの終了状態を記録するフラグ(atomicでスレッド間共有)
std::atomic<bool> send_finished(false);
std::atomic<bool> recv_finished(false);

// 受信用スレッド
void recv_thread() {
    char buf[BUFSIZE];

    while (!send_finished) { // 送信スレッドが終了したら受信も終了

        int n = recv(sock, buf, BUFSIZE - 1, 0); // サーバーからデータを受信する
                                                 // -1は"\0"を入れるために確保
        // サーバ側が接続を閉じた場合
        if (n <= 0) {
            std::cout << "\nConnection closed by server.\n";
            recv_finished = true;
            return;
        }

        buf[n] = '\0'; // 文字列終端をつける

        std::cout << "\r" << buf << "\n> ";
        std::cout.flush(); // printfを実行した結果を直ちに標準出力(画面)に放出する


    }
}

// 送信用スレッド
void send_thread() {
    std::string line; // lineという文字列クラスを宣言

    while (!recv_finished) { // 受信スレッドが終了したら送信も終了
        std::cout << "> ";
        std::getline(std::cin, line); // サーバーへ送信するメッセージを1行入力

        if (line.empty()) continue; // 空行は無視

        // サーバーへ送信
        send(sock, line.c_str(), line.size(), 0);

        //quitを送信したら終了
        if (line == "quit") {
            send_finished = true;
            return;
        }
    }
}

// メイン関数
int main(int argc, char* argv[]) {
    struct hostent* hp; // ホスト情報
    int temp = 1;

    // 引数チェック
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
        return 1;
    }

    // Winsockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    // ソケットの作成: INET ドメイン・ストリーム型
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "client: socket failed\n";
        return 1;
    }

    // サーバプロセスのソケットアドレス情報の設定
    sockaddr_in server{}; // アドレス情報構造体の初期化
    server.sin_family = AF_INET; // プロトコルファミリの設定
    server.sin_port = htons(std::stoi(argv[2])); // ポート番号の設定(文字列を整数型に変換して)

    // 文字列のIPアドレスををバイナリ形式(数値)に変換: inet_pton()関数

    if (inet_pton(AF_INET, argv[1], &server.sin_addr) <= 0) {
        std::cerr << "Invalid address\n";
        return 1;
    }

    // サーバに接続．サーバが起動し，bind(), listen() している必要あり
    if (connect(sock, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        std::cerr << "client: connect failed\n";
        return 1;
    }

    std::cout << "Connected to server.\n";

    // 送信スレッドと受信スレッドを起動
    std::thread th_recv(recv_thread);
    std::thread th_send(send_thread);

    // OSがスレッド終了を待つ
    th_send.join();
    shutdown(sock, SD_BOTH); // recv()で止まっている受信スレッドを起こす 

    th_recv.join();

    closesocket(sock); // ソケットを閉じて終了
    WSACleanup();

    std::cout << "Client terminated.\n";

    return 0;
}
