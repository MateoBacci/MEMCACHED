-module(client).
-export([start/1, monit/1, put/2, cli/1, get/1, stats/0, del/1]).
-export([input_to_bin/1, response_to_string/1]).

% Primera función del programa, crea el hilo que inicia al cliente como tal y controlará sus errores futuros. 
start(Hostname) ->
    process_flag(trap_exit, true),
    spawn(fun() -> ?MODULE:monit(Hostname) end).

% Función que crea el hilo que enviará los mensajes del cliente al servidor y replicará su respuesta.
% Al haber ejecutado process_flag(trap_exit, true), este hilo funcionará como monitor del cliente.
% Entonces crea el hilo que funciona como intermediario entre el usuario y el servidor, lo registra
% como client y se mantiene esperando a que falle. En ese caso vuelve a llamarse y de esa forma 
% vuelve a levantar la conexión con el servidor.
monit(Hostname) ->
    case gen_tcp:connect(Hostname, 8889, [binary, {packet, 0}, {active, false}]) of
        {ok, Socket} ->
            try
                register(client, spawn_link(client, cli, [Socket])),
                io:fwrite("Conexión con el servidor exitosa~n")
            catch
                error:_ ->  client ! {stop, self()},
                            unregister(client),
                            register(client, spawn_link(client, cli, [Socket])),
                            io:fwrite("Conexión con el servidor exitosa~n")
            end;
        _ -> io:fwrite("Error al conectarse con el servidor.~n")
    end,
    receive
        {'EXIT', Pid, _} -> io:fwrite("Problema con ~p~n", [Pid]),
                            ?MODULE:monit(Hostname)
    end.

% Convierte la entrada del usuario a su codificación en binario para ser enviada al servidor.
input_to_bin(Input) ->
    BInput = term_to_binary(Input),
    <<131, Tag:8/integer, _/binary>> = BInput,
    case Tag of
        107 -> BOutput = list_to_binary(Input), % Si es un string, lo convierte directamente sin formato.
               Len = byte_size(BOutput);
        _   -> BOutput = BInput,
               Len = byte_size(BOutput)
    end,
    <<Len:32/big-signed-integer-unit:1, BOutput/binary>>. % Convierte el largo en un número de 32 bits en big-endian.


% Toma la respuesta del servidor y la decodifica para que pueda ser leída.
response_to_string(Input) ->
    Len = byte_size(Input),
    case Len of 
        1 -> if Input == <<101>> -> ok;
                Input == <<111>> -> einval;
                Input == <<112>> -> enotfound;
                Input == <<113>> -> ebinary;
                Input == <<114>> -> ebig;
                Input == <<115>> -> eunk
             end;
        _ -> try 
                <<101, _Size:4/binary, Tag:1/binary, Msg/binary>> = Input,
                case Tag of
                    <<131>> -> {ok, binary_to_term(<<Tag:1/binary, Msg/binary>>)};
                    _       -> {ok, binary_to_list(<<Tag:1/binary, Msg/binary>>)}
                end
            catch
                error:_ -> {error, "Problema con la conexión al servidor. Volver a intentar"}
            end
    end.

% Función principal del cliente. Es un loop infinito. Espera a que le llegue algún mensaje (que lo 
% envía el usuario ejecutando put/2, del/1, get/1 o stats/0). Codifica dicho mensaje en binario y se
% lo envía al servidor. Finalmente, reenvía la respuesta del servidor al pid que le envió el primer 
% mensaje.
cli(Socket) -> 
    receive 
        {put, K, V, Pid} -> <<KLen:32/big-signed-integer-unit:1, KData/binary>> = input_to_bin(K),
                            BValue = input_to_bin(V),
                            Bin = <<11, KLen:32/big-signed-integer-unit:1, KData:KLen/binary, BValue/binary>>;
        {del, K, Pid} -> BInput = input_to_bin(K),
                         Bin = <<12, BInput/binary>>;
        {get, K, Pid} -> BInput = input_to_bin(K),
                         Bin = <<13, BInput/binary>>;
        {stats, Pid} -> Bin = <<21>>;
        {stop, Pid}  -> Bin = <<0>>,
                        exit(0)
    end,
    gen_tcp:send(Socket, Bin),
    case gen_tcp:recv(Socket, 0, 2000) of
        {ok, Response} -> Pid ! {ok, Response};
        {error, Reason} -> io:fwrite("Error recibiendo mensaje: ~p~n", [Reason]),
                           Pid ! {error, Reason}
    end,
    cli(Socket).

% Envía el comando al cliente, espera su respuesta y la codifica para mostrarla en pantalla. 
put(K, V) ->
    client ! {put, K, V, self()},
    receive
        {ok, Response} -> RespBin = response_to_string(Response);
        {error, _Reason} -> RespBin = error
    end,
    RespBin.

% Envía el comando al cliente, espera su respuesta y la codifica para mostrarla en pantalla. 
get(K) ->
    client ! {get, K, self()},
    receive
        {ok, Response} -> RespBin = response_to_string(Response);
        {error, _Reason} -> RespBin = error
    end,
    RespBin.

% Envía el comando al cliente, espera su respuesta y la codifica para mostrarla en pantalla. 
del(K) ->
    client ! {del, K, self()},
    receive
        {ok, Response} -> RespBin = response_to_string(Response);
        {error, _Reason} -> RespBin = error
    end,
    RespBin.

% Envía el comando al cliente, espera su respuesta y la codifica para mostrarla en pantalla. 
stats() ->
    client ! {stats, self()},
    receive
        {ok, Response} -> RespBin = response_to_string(Response);
        {error, _Reason} -> RespBin = error
    end,
    RespBin.

